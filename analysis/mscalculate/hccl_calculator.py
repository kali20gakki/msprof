#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
import logging
import os
from typing import List
from collections import defaultdict

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from mscalculate.interface.icalculator import ICalculator
from msmodel.hccl.hccl_model import HcclViewModel
from profiling_bean.db_dto.hccl_dto import HcclDto
from msconfig.config_manager import ConfigManager
from common_func.msprof_exception import ProfException
from common_func.msprof_iteration import MsprofIteration
from common_func.ms_constant.number_constant import NumberConstant


class HcclCalculator(ICalculator, MsMultiProcess):
    """
    Class to calculate hccl communication data and statistic data
    """
    TABLE_PATH = ConfigManager.TABLES

    def __init__(self, file_list, sample_config):
        super().__init__(sample_config)
        self._file_list = file_list
        self.sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._model = HcclViewModel(self._project_path, DBNameConstant.DB_HCCL,
                                    [DBNameConstant.TABLE_HCCL_OP, DBNameConstant.TABLE_HCCL_TASK])
        self._hccl_data = []
        self._hccl_op_counter = 0

        self.iter_range = self.sample_config.get(StrConstant.PARAM_ITER_ID)
        self.device_id = self.sample_config.get(StrConstant.PARAM_DEVICE_ID)
        self.conn = None
        self.curs = None

    @staticmethod
    def update_bandwidth(communication_data: List[HcclDto]):
        size_key = 'size(Byte)'
        for data in communication_data:
            size = data.args.get(size_key, 0)
            time = data.duration
            if time == 0:
                bandwidth = 0
            else:
                bandwidth = size / time  # 10^9 / 10^9 = 1 scale(GB/s)
            data.args['bandwidth(GB/s)'] = bandwidth

    @staticmethod
    def _get_hccl_op_report_sql() -> str:
        sql = f'select op_type, count(op_type), sum(duration) as total_time, '\
              f'min(duration) as min, sum(duration)/count(op_type) as avg, ' \
              f'max(duration) as max from {DBNameConstant.TABLE_HCCL_OP_TIME} group by op_type'
        return sql

    @staticmethod
    def _cal_total(type_time: dict) -> int:
        """
        calculate total time for each device
        :param type_time: {"op":{'op_type': 0,'count': 0,'duration': 0,
        'min': 0,'avg': 0,max': 0}}
        :return: total time
        """
        total_time = 0
        for ops in type_time.values():
            total_time += ops.get("duration", 0)
        return total_time

    def calculate(self: any) -> None:
        if not self._judge_calculate_again():
            return
        hccl_data = self._get_hccl_data()
        if hccl_data:
            self._generate_hccl_op_info(hccl_data)

    def save(self: any) -> None:
        if not self._hccl_data:
            return
        with self._model as hccl_model:
            hccl_model.rebuild_hccl_table()
            hccl_model.insert_data_to_db(DBNameConstant.TABLE_HCCL_ALL_REDUCE, self._hccl_data)

    def ms_run(self: any) -> None:
        """
        entry
        :return: None`
        """
        if not os.path.exists(PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL)):
            return
        self.calculate()
        self.save()
        self.process()

    def _judge_calculate_again(self):
        if not ProfilingScene().is_operator():
            logging.info("In graph scene, to generate table %s", DBNameConstant.TABLE_HCCL_ALL_REDUCE)
            return True
        else:
            hccl_db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL)
            if DBManager.check_tables_in_db(hccl_db_path, DBNameConstant.TABLE_HCCL_ALL_REDUCE):
                logging.info("Found table %s in operator scene, no need to generate again",
                             DBNameConstant.TABLE_HCCL_ALL_REDUCE)
                return False
            logging.info("No table %s found, to generate it", DBNameConstant.TABLE_HCCL_ALL_REDUCE)
            return True

    def _generate_hccl_op_info(self, hccl_data: List[HcclDto]):
        for data in hccl_data:
            self._hccl_data.append([data.model_id, data.index_id, data.op_name, data.iteration,
                                    data.hccl_name, data.first_timestamp, data.plane_id, data.timestamp,
                                    data.duration, data.is_dynamic, data.task_type, data.op_type, str(data.args)])

    def _get_hccl_data(self):
        if not os.path.exists(PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL)):
            return []
        with self._model as hccl_model:
            if not hccl_model.check_table():
                logging.warning("The HCCL table does not exist, so there is no need to continue associating operators.")
                return []
            communication_data = hccl_model.get_hccl_communication_data()
            if not communication_data:
                return communication_data
            self.update_bandwidth(communication_data)
            return communication_data

    def process(self: any) -> None:
        """
        process of calculating statistic data
        :return: None
        """
        if not self._judge_precess_again():
            return
        self.create_and_insert_db()

    def _judge_precess_again(self: any) -> bool:
        if not ProfilingScene().is_operator():
            logging.info("In graph scene, to generate table %s", DBNameConstant.TABLE_HCCL_OP_REPORT)
            return True
        else:
            hccl_db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL)
            if DBManager.check_tables_in_db(hccl_db_path, DBNameConstant.TABLE_HCCL_OP_REPORT):
                logging.info("Found table %s in operator scene, no need to generate again",
                             DBNameConstant.TABLE_HCCL_OP_REPORT)
                return False
            logging.info("No table %s found, to generate it", DBNameConstant.TABLE_HCCL_OP_REPORT)
            return True

    def create_and_insert_db(self: any) -> None:
        if not self._is_table_need_to_create():
            logging.warning("No need to create table for hccl_op_report,"
                            "maybe the data of framework or task is not collected.")
            return
        map_path = self.TABLE_PATH
        self.create_table(map_path)
        self._create_time_data()
        self._create_report()
        DBManager.destroy_db_connect(self.conn, self.curs)

    def _is_table_need_to_create(self: any) -> bool:
        hccl_db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL)
        return DBManager.check_tables_in_db(hccl_db_path, DBNameConstant.TABLE_HCCL_OP)

    def create_table(self: any, map_path: str) -> None:
        self.conn, self.curs = DBManager.create_connect_db(
            PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL))
        if not self.conn or not self.curs:
            logging.error("unable to create hccl db connection")
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)

        hccl_time_create_sql = DBManager.sql_create_general_table("HcclTimeMap", DBNameConstant.TABLE_HCCL_OP_TIME,
                                                                    map_path)
        DBManager.execute_sql(self.conn, hccl_time_create_sql)

        hccl_report_create_sql = DBManager.sql_create_general_table("HcclReportMap",
                                                                     DBNameConstant.TABLE_HCCL_OP_REPORT,
                                                                     map_path)
        DBManager.execute_sql(self.conn, hccl_report_create_sql)

    def _create_time_data(self: any) -> None:
        """
        obtain the hccl_op_time table
        :return: None
        """
        if self.conn and self.curs and DBManager.judge_table_exist(self.curs, DBNameConstant.TABLE_HCCL_OP):
            hccl_time_data = self._get_hccl_time_data()
            if hccl_time_data:
                insert_sql = "insert into {} values({value})".format(DBNameConstant.TABLE_HCCL_OP_TIME,
                                                                     value='?,' * (len(hccl_time_data[0]) - 1) + '?')
                DBManager.executemany_sql(self.conn, insert_sql, hccl_time_data)

    def _get_hccl_time_data(self: any) -> list:
        """
        obtain the hccl_time_data
        :return: None
        """
        hccl_time_data = []
        iter_list = MsprofIteration(self._project_path).get_index_id_list_with_index_and_model(self.iter_range)
        for index_and_model in iter_list:
            hccl_time_sql = f'select op_type, begin, end, (end-begin) as duration '\
                            f'from {DBNameConstant.TABLE_HCCL_OP} where index_id=? and model_id=?'
            hccl_time_data.extend(DBManager.fetch_all_data(self.curs, hccl_time_sql, index_and_model))
        return hccl_time_data

    def _create_report(self: any) -> None:
        """
        create report table
        :return: None
        """
        sql = self._get_hccl_op_report_sql()
        task_data = DBManager.fetch_all_data(self.curs, sql)

        if not task_data:
            return
        type_time = defaultdict(dict)

        for task in task_data:
            type_time[task[0]] = {
                'op_type': task[0], 'count': task[1], 'duration': task[2],
                'min': task[3], 'avg': task[4], 'max': task[5]
            }
        total_time = self._cal_total(type_time)
        total_data = []
        for op_type in type_time:
            task_data = type_time[op_type]
            try:
                task_duration_ratio = round(float(task_data["duration"] / total_time * 100),
                                      NumberConstant.DECIMAL_ACCURACY)
            except ZeroDivisionError:
                task_duration_ratio = 0
            total_data.append(
                (task_data['op_type'],
                 task_data["count"],
                 round(float(task_data["duration"]), NumberConstant.DECIMAL_ACCURACY),
                 round(float(task_data["min"]), NumberConstant.DECIMAL_ACCURACY),
                 round(float(task_data["avg"]), NumberConstant.DECIMAL_ACCURACY),
                 round(float(task_data["max"]), NumberConstant.DECIMAL_ACCURACY),
                 task_duration_ratio))
        if total_data:
            sorted_total_data = sorted(total_data, key=lambda x: x[5], reverse=True)
            sql = 'insert into {} values({})'.format(DBNameConstant.TABLE_HCCL_OP_REPORT,
                                                     '?,' * (len(sorted_total_data[0]) - 1) + '?')
            DBManager.executemany_sql(self.conn, sql, sorted_total_data)

