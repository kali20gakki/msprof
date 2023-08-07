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
from common_func.ms_constant.number_constant import NumberConstant


class HcclCalculator(ICalculator, MsMultiProcess):
    """
    Class to calculate hccl communication data and statistic data
    """
    TABLE_PATH = ConfigManager.TABLES

    def __init__(self, file_list, sample_config):
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._model = HcclViewModel(self._project_path, DBNameConstant.DB_HCCL,
                                    [DBNameConstant.TABLE_HCCL_OP, DBNameConstant.TABLE_HCCL_TASK])
        self._hccl_data = []
        self._hccl_op_report_data = []
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
        hccl_data, hccl_op_report_data = self._get_hccl_data()
        if hccl_data:
            self._generate_hccl_op_info(hccl_data)
            self._create_report(hccl_op_report_data)

    def save(self: any) -> None:
        if not self._hccl_data:
            return
        with self._model as hccl_model:
            hccl_model.rebuild_hccl_table()
            hccl_model.insert_data_to_db(DBNameConstant.TABLE_HCCL_ALL_REDUCE, self._hccl_data)
            hccl_model.rebuild_hccl_op_report_table()
            hccl_model.insert_data_to_db(DBNameConstant.TABLE_HCCL_OP_REPORT, self._hccl_op_report_data)

    def ms_run(self: any) -> None:
        """
        entry
        :return: None`
        """
        if not os.path.exists(PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL)):
            return
        if not self._judge_calculate_again():
            return
        self.calculate()
        self.save()

    def _judge_calculate_again(self):
        if not ProfilingScene().is_operator():
            logging.info("In graph scene, to generate table %s and %s", DBNameConstant.TABLE_HCCL_ALL_REDUCE,
                         DBNameConstant.TABLE_HCCL_OP_REPORT)
            return True
        else:
            hccl_db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL)
            if DBManager.check_tables_in_db(hccl_db_path, DBNameConstant.TABLE_HCCL_ALL_REDUCE,
                                            DBNameConstant.TABLE_HCCL_OP_REPORT):
                logging.info("Found table %s and %s in operator scene, no need to generate again",
                             DBNameConstant.TABLE_HCCL_ALL_REDUCE, DBNameConstant.TABLE_HCCL_OP_REPORT)
                return False
            logging.info("No table %s or %s found, to generate it", DBNameConstant.TABLE_HCCL_ALL_REDUCE,
                         DBNameConstant.TABLE_HCCL_OP_REPORT)
            return True

    def _generate_hccl_op_info(self, hccl_data: List[HcclDto]):
        for data in hccl_data:
            self._hccl_data.append([data.model_id, data.index_id, data.op_name, data.iteration,
                                    data.hccl_name, data.first_timestamp, data.plane_id, data.timestamp,
                                    data.duration, data.is_dynamic, data.task_type, data.op_type, str(data.args)])

    def _get_hccl_data(self):
        with self._model as hccl_model:
            if not hccl_model.check_table():
                logging.warning("The HCCL table does not exist, so there is no need to continue associating operators.")
                return [], []
            communication_data = hccl_model.get_hccl_communication_data()
            if not communication_data:
                return [], []
            report_data = hccl_model.get_hccl_op_report_data()
            self.update_bandwidth(communication_data)
            return communication_data, report_data

    def _create_report(self, hccl_op_report_data: List[list]) -> None:
        """
        calculate report data
        :return: None
        """
        task_data = hccl_op_report_data
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
            task_data = type_time.get(op_type, {})
            if not task_data:
                continue
            task_duration_ratio = round(float(task_data["duration"] / total_time * 100),
                                        NumberConstant.DECIMAL_ACCURACY) if total_time != 0 else 0
            total_data.append(
                (task_data['op_type'],
                 task_data["count"],
                 round(float(task_data["duration"]), NumberConstant.DECIMAL_ACCURACY),
                 round(float(task_data["min"]), NumberConstant.DECIMAL_ACCURACY),
                 round(float(task_data["avg"]), NumberConstant.DECIMAL_ACCURACY),
                 round(float(task_data["max"]), NumberConstant.DECIMAL_ACCURACY),
                 task_duration_ratio))
        if total_data:
            self._hccl_op_report_data = sorted(total_data, key=lambda x: x[5], reverse=True)