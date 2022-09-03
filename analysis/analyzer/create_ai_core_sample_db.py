#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.

import logging
import os
import sqlite3
import struct
from abc import abstractmethod
from collections import OrderedDict

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.file_manager import FileManager, FileOpen
from common_func.file_name_manager import FileNameManagerConstant
from common_func.file_name_manager import get_ai_core_compiles
from common_func.file_name_manager import get_aiv_compiles
from common_func.file_name_manager import get_file_name_pattern_match
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import MsvpCommonConst
from common_func.msvp_common import config_file_obj
from common_func.msvp_common import error
from common_func.msvp_common import is_valid_original_data
from common_func.msvp_common import read_cpu_cfg
from common_func.path_manager import PathManager
from common_func.utils import Utils
from mscalculate.calculate_ai_core_data import CalculateAiCoreData
from viewer.calculate_rts_data import check_aicore_events


class ParsingCoreSampleData(MsMultiProcess):
    """
    base parsing sample data class
    """

    TABLE_PATH = os.path.join(MsvpCommonConst.CONFIG_PATH, 'Tables.ini')
    TYPE = "ai_core"
    BYTE_ORDER_CHAR = '='
    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self.sample_config = sample_config
        self.curs = None
        self.conn = None
        self.ai_core_data = []
        self.device_list = []
        self.device_id = 0
        self.replayid = 0

    @staticmethod
    def get_ai_core_event_chunk(event: list) -> list:
        """
        get ai core event chunk
        """
        check_aicore_events(event)
        ai_core_events = Utils.generator_to_list(event[i:i + 8]
                                                 for i in range(0, len(event), 8))
        return ai_core_events

    @staticmethod
    def __generate_event_num(event_sum: dict) -> dict:
        return {key: value + [key] for key, value in event_sum.items()}

    @abstractmethod
    def ms_run(self: any) -> None:
        """
        implement in child class
        :return:
        """

    def sql_insert_metric_summary_table(self: any, metrics: list, freq: float, metric_key: str) -> str:
        """
        generate sql statement for inserting metric from EventCount
        :param metric_key: aic or aiv profiling metric
        :param metrics: metrics to be calcualted
        :param freq: running frequecy, which can be used to calculate aic metrics
        :return: merged sql sentence
        """
        algos = []
        cal = CalculateAiCoreData(self.sample_config.get("result_dir"))
        cal.add_fops_header(metric_key, metrics)
        field_dict = read_cpu_cfg(self.TYPE, 'formula')
        field_dict_collection = []
        for i in metrics:
            field_dict_collection.append((i, field_dict.get(i.replace("(GB/s)", "(gb/s)"))
                                          .replace('/block_num*((block_num+core_num-1)/core_num)', '')))
        field_dict = OrderedDict(field_dict_collection)
        for field in field_dict:
            algo = field_dict.get(field)
            algo = algo.replace("freq", str(freq))
            algo = cal.update_fops_data(field, algo)
            algos.append(algo)

        sql = "SELECT " + ",".join(Utils.generator_to_list("cast(" + algo + " as decimal(8,2))"
                                                           for algo in algos)) + " FROM EventCount " \
                                                                                 "where coreid = ?"
        return sql

    def read_binary_data(self: any, binary_data_path: str) -> None:
        """
        read binary data
        """
        project_path = self.sample_config.get("result_dir")
        delta_dev = InfoConfReader().get_delta_time()
        file_name = PathManager.get_data_file_path(project_path, binary_data_path)
        try:
            with FileOpen(file_name, 'rb') as file_:
                self.__read_binary_data_helper(file_.file_reader, delta_dev)
                self.insert_binary_data(file_name)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error("%s: %s", binary_data_path, err)

    def insert_binary_data(self: any, file_name: str) -> None:
        """
        insert binary data
        """
        try:
            self.insert_data(file_name)
        except sqlite3.Error as err:
            logging.error("%s: %s", file_name, err)
        finally:
            del self.ai_core_data[:]

    def insert_metric_value(self: any) -> int:
        """
        insert metric value
        """
        metrics_config = read_cpu_cfg(self.TYPE, "formula")
        if not metrics_config:
            return NumberConstant.ERROR
        sql = "CREATE TABLE IF NOT EXISTS MetricSummary (metric text, " \
              "value numeric, coreid INT)"
        DBManager.execute_sql(self.conn, sql)
        core_id = DBManager.fetch_all_data(self.curs, "select distinct(coreid) from EventCount;")
        insert_data = []
        for core in core_id:
            for key in list(metrics_config.keys()):
                insert_data.append([key.replace("(gb/s)", "(GB/s)"), None, core[0]])
        DBManager.insert_data_into_table(self.conn, "MetricSummary", insert_data)
        return NumberConstant.SUCCESS

    def insert_metric_summary_table(self: any, freq: float, metric_key: str) -> None:
        """
        insert metric summary table
        """
        metrics = ['total_time(ms)']
        sample_metrics = self.__check_has_metrics(metric_key)
        if not sample_metrics:
            return

        sample_metrics_lst = sample_metrics.split(",")
        try:
            self.__check_is_valid_metrics(sample_metrics_lst)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            error(self.FILE_NAME, str(err))
            return
        metrics.extend(sample_metrics_lst)
        logging.info('start insert into MetricSummary')
        core_id = DBManager.fetch_all_data(self.curs, "select distinct(coreid) from EventCount;")
        sql = self.sql_insert_metric_summary_table(metrics, freq, metric_key)
        self.__insert_metric_summary_table_one_by_one(core_id, sql, metrics)

    def insert_data(self: any, file_name: str) -> None:
        """
        insert data
        """
        if self.ai_core_data:
            logging.info(
                'start insert into AICoreOriginalData: %s',
                file_name)
            if not DBManager.judge_table_exist(self.curs, 'AICoreOriginalData'):
                self.create_ai_core_original_datatable(
                    'AICoreOriginalDataMap')

            count_num = self.ai_core_data[0][0]
            column = 'mode,replayid,timestamp,coreid,task_cyc,'
            column = column + ','.join(('event{}'.format(i) for i in range(1, count_num + 1)))
            sql = 'insert into AICoreOriginalData ({column}) values ({value})'. \
                format(column=column, value='?,' * (4 + count_num) + '?')
            DBManager.executemany_sql(self.conn, sql, (x[1:] for x in self.ai_core_data))

    def create_ai_core_original_datatable(self: any, table_name: str) -> None:
        """
        create table
        """
        sql = DBManager.sql_create_general_table(
            table_name, 'AICoreOriginalData', self.TABLE_PATH)
        if not sql:
            logging.error("generate sql statement failed!")
            error(self.FILE_NAME, "generate sql statement failed!")
            DBManager.destroy_db_connect(self.conn, self.curs)
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        DBManager.execute_sql(self.conn, sql)

    def create_ai_event_tables(self: any, events: list) -> int:
        """
        create ai event tables
        """
        try:
            return self.__create_ai_event_tables_helper(events)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return NumberConstant.ERROR

    def __insert_metric_summary_table_one_by_one(self: any, core_id: list, sql: str, metrics: list) -> None:
        for core in core_id:
            result = DBManager.fetch_all_data(self.curs, sql)
            for row in result:
                index = 0
                for key in metrics:
                    value = row[index] if row[index] else 0
                    DBManager.execute_sql(self.conn, "UPDATE MetricSummary SET value=? WHERE metric=? and coreid=?",
                                          (value, key, core[0]))
                    index += 1

    def __check_has_metrics(self: any, metric_key: str) -> str:
        if self.sample_config.get(metric_key) not in Constant.AICORE_METRICS_LIST:
            return ""
        sample_metrics = Constant.AICORE_METRICS_LIST.get(
            self.sample_config.get("ai_core_metrics"))
        if not sample_metrics:
            return ""
        return sample_metrics

    def __check_is_valid_metrics(self: any, sample_metrics_lst: list) -> None:
        for key in sample_metrics_lst:
            if key.lower() not in \
                    (item[0] for item in config_file_obj(file_name='AICore').items('formula')):
                error(self.FILE_NAME, 'Invalid metric {} .'.format(key))
                raise ProfException(ProfException.PROF_SYSTEM_EXIT)

    def __create_event_count_table(self: any, event_: list) -> None:
        check_aicore_events(event_)
        sql = "CREATE TABLE IF NOT EXISTS EventCount (" + \
              ",".join((pmu_event.replace('0x', 'r') +
                        " numeric" for pmu_event in event_)) + ",task_cyc numeric,coreid INT)"
        DBManager.execute_sql(self.conn, sql)

    def __create_event_value_table_one_by_one(self: any, ai_core_events: list) -> None:
        for replay, event in enumerate(ai_core_events):
            for index, value in enumerate(event):
                DBManager.drop_table(self.conn, value.replace('0x', 'r'))

                create_sql = "create table {tablename}(timestamp INTEGER, pmucount INTEGER, coreid INTEGER)" \
                    .format(tablename=value.replace('0x', 'r'))
                DBManager.execute_sql(self.conn, create_sql)
                sql = "insert into {tablename} select timestamp, event{index}, coreid " \
                      "from AICoreOriginalData " \
                      "where replayid = ?".format(tablename=value.replace('0x', 'r'),
                                                  index=index + 1, )
                DBManager.execute_sql(self.conn, sql, (replay,))

    def __create_task_cyc_table(self: any) -> None:
        create_sql = "create table task_cyc(timestamp INTEGER, pmucount INTEGER, coreid INTEGER)"
        DBManager.execute_sql(self.conn, create_sql)
        sql = "insert into task_cyc select timestamp, task_cyc, coreid " \
              "from AICoreOriginalData where replayid is 0"
        DBManager.execute_sql(self.conn, sql)

    def __update_event_sum(self: any, event_: list, event_sum: dict) -> None:
        for i in event_:
            sql = "select sum(pmucount),coreid from {tablename} group by coreid order by coreid" \
                .format(tablename=i.replace("0x", "r"))
            result = DBManager.fetch_all_data(self.curs, sql)
            for j in result:
                if j[-1] in event_sum:
                    event_sum[j[-1]].append(j[0])
                else:
                    event_sum[j[-1]] = [j[0]]
        result = DBManager.fetch_all_data(self.curs, "select sum(pmucount),coreid from task_cyc "
                                                     "group by coreid order by coreid")
        for j in result:
            if j[-1] in event_sum:
                event_sum[j[-1]].append(j[0])
            else:
                event_sum[j[-1]] = [j[0]]

    def __insert_into_event_count_table(self: any, event_: list, event_sum: dict) -> None:
        sql = 'insert into EventCount values ({value})'.format(value="?," * (len(event_)) + "?,?")
        DBManager.execute_sql(self.conn, sql, list(event_sum.values()))
        select_sql = "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='r11'"
        fetch_data = DBManager.fetch_all_data(self.curs, select_sql)
        if fetch_data:
            DBManager.execute_sql(self.conn, "drop table if exists cycles")
            DBManager.execute_sql(self.conn, "alter table r11 rename to cycles")

    def __read_binary_data_helper(self: any, file_: any, delta_dev: float) -> None:
        while True:
            mode_data = file_.read(1)  # One Byte data for data mode
            if mode_data:
                mode = struct.unpack(self.BYTE_ORDER_CHAR + "B", mode_data)[0]
                # Eight Byte alignment
                _ = file_.read(7)
                # Eight Byte for each pmu event. Eight events for each struct
                event_count = struct.unpack(self.BYTE_ORDER_CHAR + '8Q', file_.read(64))
                # Eight Byte for TASK_CYC_CNT
                task_cyc = struct.unpack(self.BYTE_ORDER_CHAR + 'Q', file_.read(8))[0]
                # Eight Byte for timestamp
                timestamp = struct.unpack(self.BYTE_ORDER_CHAR + 'Q', file_.read(8))[0] \
                            + delta_dev * NumberConstant.NANO_SECOND
                # One Byte for count number
                count_num = struct.unpack(self.BYTE_ORDER_CHAR + "B", file_.read(1))[0]
                # One Byte for core id
                core_id = struct.unpack(self.BYTE_ORDER_CHAR + "B", file_.read(1))[0]
                file_.read(6)  # reserved space
                tmp = [count_num, mode, self.replayid, timestamp, core_id, task_cyc]
                tmp.extend(event_count[:count_num])
                self.ai_core_data.append(tmp)
            else:
                break

    def __create_ai_event_tables_helper(self: any, events: list) -> int:
        logging.info(
            'start create ai core events tables')
        event_ = events
        self.__create_event_count_table(event_)

        event_sum = {}
        ai_core_events = self.get_ai_core_event_chunk(event_)
        if not ai_core_events:
            logging.error('get ai core pmu events failed!')
            return NumberConstant.ERROR

        self.__create_event_value_table_one_by_one(ai_core_events)
        self.__create_task_cyc_table()
        self.__update_event_sum(event_, event_sum)

        event_sum = self.__generate_event_num(event_sum)
        self.__insert_into_event_count_table(event_, event_sum)
        logging.info(
            'create event tables finished')
        return NumberConstant.SUCCESS


class ParsingAICoreSampleData(ParsingCoreSampleData):
    """
    parsing ai core data class
    """

    def __init__(self: any, sample_config: dict) -> None:
        ParsingCoreSampleData.__init__(self, sample_config)

    def start_parsing_data_file(self: any) -> None:
        """
        parsing data file
        """
        project_path = self.sample_config.get("result_dir")
        file_all = sorted(os.listdir(PathManager.get_data_dir(project_path)))
        ai_core_compiles = get_ai_core_compiles()
        for file_name in file_all:
            result = get_file_name_pattern_match(file_name, *ai_core_compiles)
            if result and is_valid_original_data(file_name, project_path):
                self.device_id = result.groups()[FileNameManagerConstant.MATCHED_DEV_ID_INX]
                self.replayid = '0'
                self.device_list.append(self.device_id)
                self.conn, self.curs = DBManager.create_connect_db(os.path.join(
                    project_path,
                    "sqlite", "aicore_" + str(self.device_id) + ".db"))
                DBManager.execute_sql(self.conn, "PRAGMA page_size=8192")
                DBManager.execute_sql(self.conn, "PRAGMA journal_mode=WAL")
                logging.info(
                    "start parsing aicore data file: %s", file_name)
                self.read_binary_data(file_name)
                FileManager.add_complete_file(project_path, file_name)
        logging.info("Create AICore DB finished!")

    def create_ai_core_db(self: any) -> None:
        """
        create aicore db
        """
        project_path = self.sample_config.get("result_dir")

        if self.sample_config.get('ai_core_profiling_mode') != 'sample-based':
            return
        self.start_parsing_data_file()
        try:
            self._process_device_list_ai_core(project_path)
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            DBManager.destroy_db_connect(self.conn, self.curs)

    def ms_run(self: any) -> None:
        """
        main entry for parsing ai core data
        :return: None
        """
        try:
            self.create_ai_core_db()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _process_device_list_ai_core(self: any, project_path: str) -> None:
        for device in set(self.device_list):
            self.conn = sqlite3.connect(os.path.join(
                project_path,
                "sqlite", "aicore_" + str(device) + ".db"))
            self.curs = self.conn.cursor()
            event_ = self.sample_config.get("ai_core_profiling_events", "").split(",")
            status = self.create_ai_event_tables(event_)
            if status == NumberConstant.ERROR:
                return
            status = self.insert_metric_value()
            if status == NumberConstant.ERROR:
                return
            freq = InfoConfReader().get_freq(StrConstant.AIC)
            self.insert_metric_summary_table(freq, StrConstant.AI_CORE_PROFILING_METRICS)


class ParsingAIVectorCoreSampleData(ParsingCoreSampleData):
    """
    parsing ai vector core data class
    """

    def __init__(self: any, sample_config: dict) -> None:
        ParsingCoreSampleData.__init__(self, sample_config)

    def start_parsing_data_file(self: any) -> None:
        """
        parsing data file
        """
        project_path = self.sample_config.get("result_dir")
        file_all = sorted(os.listdir(PathManager.get_data_dir(project_path)))
        aiv_compiles = get_aiv_compiles()
        for file_name in file_all:
            result = get_file_name_pattern_match(file_name, *aiv_compiles)
            if result:
                self.device_id = result.groups()[FileNameManagerConstant.MATCHED_DEV_ID_INX]
                self.replayid = '0'
                self.device_list.append(self.device_id)
                self.conn, self.curs = DBManager.create_connect_db(os.path.join(
                    project_path,
                    "sqlite", "ai_vector_core_" + str(self.device_id) + ".db"))
                DBManager.execute_sql(self.conn, "PRAGMA page_size=8192")
                DBManager.execute_sql(self.conn, "PRAGMA journal_mode=WAL")
                logging.info(
                    "start parsing ai vector core data file: %s", file_name)
                self.read_binary_data(file_name)
        logging.info("Create AIVectorCore DB finished!")

    def create_ai_vector_core_db(self: any) -> None:
        """
        create ai core db
        """
        project_path = self.sample_config.get("result_dir")
        if self.sample_config.get('aiv_profiling_mode') == 'sample-based':
            self.start_parsing_data_file()
            try:
                self._process_device_list_ai_vector(project_path)
            except sqlite3.Error as err:
                logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        DBManager.destroy_db_connect(self.conn, self.curs)

    def ms_run(self: any) -> None:
        """
        main entry for creating ai core db
        :return: None
        """
        try:
            self.create_ai_vector_core_db()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _process_device_list_ai_vector(self: any, project_path: str) -> None:
        for device in set(self.device_list):
            self.conn = sqlite3.connect(os.path.join(
                project_path,
                "sqlite", "ai_vector_core_" + str(device) + ".db"))
            self.curs = self.conn.cursor()
            event_ = self.sample_config.get("aiv_profiling_events", "").split(",")
            status = self.create_ai_event_tables(event_)
            if status == NumberConstant.ERROR:
                return
            status = self.insert_metric_value()
            if status == NumberConstant.ERROR:
                return
            freq = InfoConfReader().get_freq(StrConstant.AIV)
            self.insert_metric_summary_table(freq, StrConstant.AIV_PROFILING_METRICS)
