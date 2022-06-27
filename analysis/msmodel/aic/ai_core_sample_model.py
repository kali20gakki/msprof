# coding=utf-8
"""
ffts task pmu model
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

import logging
import os
import sqlite3
import sys
from collections import OrderedDict

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from common_func.msvp_common import config_file_obj
from common_func.msvp_common import error
from common_func.msvp_common import read_cpu_cfg
from common_func.path_manager import PathManager
from common_func.utils import Utils
from msmodel.interface.base_model import BaseModel
from mscalculate.calculate_ai_core_data import CalculateAiCoreData
from viewer.calculate_rts_data import check_aicore_events


class AiCoreSampleModel(BaseModel):
    """
    ffts pmu model.
    """
    FILE_NAME = os.path.basename(__file__)
    TYPE = "ai_core"

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.metrics_data = []
        self.conn = None
        self.cur = None

    @staticmethod
    def get_ai_core_event_chunk(event: list) -> list:
        """
        get ai core event chunk
        :param event: ai core event
        :return:
        """
        check_aicore_events(event)
        ai_core_events = Utils.generator_to_list(event[i:i + 8] for i in range(0, len(event), 8))
        return ai_core_events

    def init(self: any) -> bool:
        """
        create db and tables
        """

        self.conn, self.cur = DBManager.create_connect_db(
            PathManager.get_db_path(self.result_dir, self.db_name))
        if not (self.conn and self.cur):
            return False
        self.cur.execute("PRAGMA page_size=8192")
        self.cur.execute("PRAGMA journal_mode=WAL")
        return True

    def create_aicore_originaldatatable(self: any, table_name: str) -> None:
        """
        create table
        :param table_name:table name
        :return:
        """
        sql = DBManager.sql_create_general_table(
            table_name, 'AICoreOriginalData', self.TABLES_PATH)
        if not sql:
            logging.error("generate sql statement failed!")
            error(self.FILE_NAME, "generate sql statement failed!")
            DBManager.destroy_db_connect(self.conn, self.cur)
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        DBManager.execute_sql(self.conn, sql)

    def create_core_table(self: any, events: list, ai_core_data: list) -> None:
        """
        create ai event tables
        :param events: ai core events
        :param ai_core_data: ai core data
        :return:
        """
        if not DBManager.judge_table_exist(self.cur, 'AICoreOriginalData'):
            self.create_aicore_originaldatatable('AICoreOriginalDataMap')
        self.flush(ai_core_data)
        logging.info("start create ai core events tables")
        try:
            if self._init_ai_core_events_table(events):
                self._init_task_cyc_table()
                self._insert_event_table(events)
                logging.info('create event tables finished')
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err))
        finally:
            pass

    def insert_metric_summary_table(self: any, freq: int, key: str) -> None:
        """
        insert metric summary table
        :param key: key of metric
        :param freq: Frequency
        :return:
        """
        try:
            metrics = self._get_metrics(key)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err))
            error(self.FILE_NAME, str(err))
            return
        if metrics:
            self._do_insert_metric_summary_table(freq, metrics)

    def insert_metric_value(self: any) -> int:
        """
        insert metric value
        :return:
        """
        metrics_config = read_cpu_cfg(self.TYPE, "formula")
        data = []
        if metrics_config:
            sql = "CREATE TABLE IF NOT EXISTS MetricSummary (metric text, " \
                  "value numeric, coreid INT)"
            DBManager.execute_sql(self.conn, sql)
            core_id = DBManager.fetch_all_data(self.cur, "select distinct(coreid) from EventCount;")
            for core in core_id:
                data.extend([key.replace("(gb/s)", "(GB/s)"), None, core[0]] for key in list(metrics_config.keys()))
            DBManager.insert_data_into_table(self.conn, "MetricSummary", data)
            return NumberConstant.SUCCESS
        return NumberConstant.ERROR

    def sql_insert_metric_summary_table(self: any, metrics: list, freq: float, metric_key: str) -> str:
        """
        generate sql statement for inserting metric from EventCount
        :param metrics: metrics to be calcualted
        :param freq: running frequecy, which can be used to calculate aic metrics
        :param metric_key: metric key
        :return: merged sql sentence
        """
        algos = []
        cal = CalculateAiCoreData(self.result_dir)
        cal.add_fops_header(metric_key, metrics)
        field_dict = read_cpu_cfg(self.TYPE, 'formula')
        res = []
        for metric in metrics:
            replaced_metric = metric.replace("(GB/s)", "(gb/s)")
            field_val = field_dict[replaced_metric].replace('/block_num*((block_num+core_num-1)/core_num)', '')
            res.append((replaced_metric, field_val))
        field_dict = OrderedDict(res)
        for field in field_dict:
            algo = field_dict[field]
            algo = algo.replace("freq", str(freq))
            algo = cal.update_fops_data(field, algo)
            algos.append(algo)

        sql = "SELECT " + ",".join("cast(" + algo + " as decimal(8,2))"
                                   for algo in algos) + " FROM EventCount where coreid = ?"
        return sql

    def flush(self: any, data_list: list) -> None:
        """
        insert data into database
        :param data_list: ffts pmu data list
        :return: None
        """
        count_num = data_list[0][0]
        column = 'mode,replayid,timestamp,coreid,task_cyc,'
        column = column + ','.join('event{}'.format(i) for i in range(1, count_num + 1))
        sql = 'insert into AICoreOriginalData ({column}) values ({value})'. \
            format(column=column, value='?,' * (4 + count_num) + '?')
        DBManager.executemany_sql(self.conn, sql, Utils.generator_to_list(x[1:] for x in data_list))

    def clear(self: any) -> None:
        """
        clear ai core table
        :return: None
        """
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_RUNTIME)
        if DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY):
            DBManager.drop_table(self.conn, DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY)

    def _create_event_count_table(self: any, events: list) -> None:
        sql = "CREATE TABLE IF NOT EXISTS EventCount (" + \
              ",".join(pmu_event.replace('0x', 'r') +
                       " numeric" for pmu_event in events) + ",task_cyc numeric,coreid INT)"
        self.cur.execute(sql)
        self.conn.commit()

    def _init_ai_core_events_table(self: any, events: list) -> bool:
        check_aicore_events(events)
        self._create_event_count_table(events)

        ai_core_events = self.get_ai_core_event_chunk(events)
        if not ai_core_events:
            logging.error('get ai core pmu events failed!')
            return False
        for replay, event in enumerate(ai_core_events):
            for index, value in enumerate(event):
                DBManager.drop_table(self.conn, value.replace('0x', 'r'))
                DBManager.execute_sql(self.conn, "create table {tablename}(timestamp INTEGER, "
                                                 "pmucount INTEGER, coreid INTEGER)"
                                      .format(tablename=value.replace('0x', 'r')))
                sql = "insert into {tablename} select timestamp, event{index}, coreid " \
                      "from AICoreOriginalData " \
                      "where replayid = ?".format(tablename=value.replace('0x', 'r'),
                                                  index=index + 1, )
                DBManager.execute_sql(self.conn, sql, (replay,))
        return True

    def _get_ai_core_event_sum(self: any, events: list) -> dict:
        event_sum = {}
        for i in events:
            result = self.cur.execute("select sum(pmucount),coreid from {tablename} "
                                      "group by coreid order by coreid".
                                      format(tablename=i.replace("0x", "r"))).fetchall()
            for j in result:
                if j[-1] in event_sum:
                    event_sum.get(j[-1]).append(j[0])
                else:
                    event_sum[j[-1]] = [j[0]]
        result = self.cur.execute("select sum(pmucount),coreid from task_cyc "
                                  "group by coreid order by coreid").fetchall()
        for j in result:
            if j[-1] in event_sum:
                event_sum.get(j[-1]).append(j[0])
            else:
                event_sum[j[-1]] = [j[0]]

        event_sum = {key: value + [key] for key, value in event_sum.items()}
        return event_sum

    def _init_task_cyc_table(self: any) -> None:
        self.cur.execute("create table task_cyc(timestamp INTEGER, "
                         "pmucount INTEGER, coreid INTEGER)")
        sql = "insert into task_cyc select timestamp, task_cyc, coreid " \
              "from AICoreOriginalData where replayid is 0"
        self.cur.execute(sql)

    def _insert_event_table(self: any, events: list) -> None:
        event_sum = self._get_ai_core_event_sum(events)
        sql = 'insert into EventCount values ({value})'.format(
            value="?," * (len(events)) + "?,?")
        self.cur.executemany(sql, list(event_sum.values()))
        if (self.cur.execute("SELECT count(*) FROM sqlite_master "
                             "WHERE type='table' AND name='r11'").fetchall()[0][0]):
            self.cur.execute("drop table if exists cycles")
            self.cur.execute("alter table r11 rename to cycles")
        self.conn.commit()

    def _do_insert_metric_summary_table(self: any, freq: int, metrics: list) -> None:
        logging.info('start insert into MetricSummary')
        core_id = DBManager.fetch_all_data(self.cur, "select distinct(coreid) from EventCount;")
        sql = self.sql_insert_metric_summary_table(metrics, freq, StrConstant.AI_CORE_PROFILING_METRICS)
        for core in core_id:
            result = DBManager.fetch_all_data(self.cur, sql, (core[0],))
            for row in result:
                index = 0
                for _metric in metrics:
                    value = row[index] if row[index] else 0
                    DBManager.execute_sql(self.conn, "UPDATE MetricSummary SET value=? WHERE metric=? and coreid=?",
                                          (value, _metric, core[0]))
                    index += 1

    def _get_metrics(self: any, key: str) -> list:
        metrics = ['total_time(ms)']
        if key not in Constant.AICORE_METRICS_LIST:
            return []
        sample_metrics = Constant.AICORE_METRICS_LIST.get(key)
        if not sample_metrics:
            return []
        sample_metrics_lst = sample_metrics.split(",")
        for _key in sample_metrics_lst:
            if _key.lower() not in \
                    (item[0] for item in config_file_obj(file_name='AICore').items('formula')):
                error(self.FILE_NAME, 'Invalid metric {} .'.format(_key))
                raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        metrics.extend(sample_metrics_lst)
        return metrics
