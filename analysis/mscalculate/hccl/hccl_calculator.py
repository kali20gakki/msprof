#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.

import logging
import os
from collections import defaultdict
from collections import deque
from typing import List

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from mscalculate.hccl.hccl_task import HcclOps
from mscalculate.hccl.hccl_task import HcclTask
from mscalculate.interface.icalculator import ICalculator
from msconfig.config_manager import ConfigManager
from msmodel.hccl.hccl_model import HcclViewModel
from profiling_bean.db_dto.step_trace_dto import IterationRange


class HcclCalculator(ICalculator, MsMultiProcess):
    """
    Class to calculate hccl communication data and statistic data
    """
    TABLE_PATH = ConfigManager.TABLES

    def __init__(self, file_list, sample_config):
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._model = HcclViewModel(self._project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE,
                                    [DBNameConstant.TABLE_HCCL_SINGLE_DEVICE, DBNameConstant.TABLE_HCCL_OP_REPORT])
        self._hccl_data = []
        self._hccl_op_report_data = []

    @staticmethod
    def update_bandwidth(communication_data: List[HcclTask]):
        for index, data in enumerate(communication_data):
            if data.duration == 0:
                bandwidth = 0
            else:
                bandwidth = data.size / data.duration  # 10^9 / 10^9 = 1 scale(GB/s)
            communication_data[index] = data.replace(bandwidth=bandwidth)

    @staticmethod
    def update_op_name_by_group_name(communication_data: List[HcclTask]):
        group_dict = defaultdict(lambda: {"first_timestamp": 0, "count": 0})
        for num, data in enumerate(communication_data):
            if data.group_name in group_dict:
                if data.first_timestamp > group_dict[data.group_name]["first_timestamp"]:
                    group_dict[data.group_name]["first_timestamp"] = data.first_timestamp
                    group_dict[data.group_name]["count"] += 1
            else:
                group_dict[data.group_name]["first_timestamp"] = data.first_timestamp
                group_dict[data.group_name]["count"] = 0
            index = group_dict[data.group_name]["count"]
            communication_data[num] = data.replace(
                op_name=f"{data.op_name}_{data.group_name[-3:]}_{str(index)}_{str(data.iter_id)}")

    @staticmethod
    def _cal_total(type_time: dict) -> int:
        """
        calculate total time for each device
        :param type_time: {"op_type":{'count': 0,'duration': 0, 'min': 0,'avg': 0,max': 0}}
        :return: total time
        """
        total_time = 0
        for ops in type_time.values():
            total_time += ops.get("total_time", 0)
        return total_time

    @staticmethod
    def _get_hccl_op_report_data(communication_data: List[HcclTask]) -> any:
        """
        Calculate the hccl op report data by communication data
        return：{"op_type":{'count': 0,'duration': 0, min': 0,'avg': 0,max': 0}}
        """
        if not communication_data:
            return {}
        grouped_data = defaultdict(lambda: {"min_timestamp": float("inf"), "max_timestamp": -float("inf")})
        for data in communication_data:
            key = (data.op_name, data.first_timestamp, data.op_type)
            grouped_data[key]["min_timestamp"] = min(grouped_data[key]["min_timestamp"], data.timestamp)
            grouped_data[key]["max_timestamp"] = max(grouped_data[key]["max_timestamp"], data.timestamp + data.duration)
            grouped_data[key]["op_type"] = data.op_type
        for value in grouped_data.values():
            value["duration"] = value["max_timestamp"] - value["min_timestamp"]

        op_type_group = defaultdict(lambda: {"count": 0, "total_time": 0, "min": float("inf"), "max": -float("inf")})
        for entry in grouped_data.values():
            op_type_status = op_type_group[entry["op_type"]]
            op_type_status["count"] += 1
            op_type_status["total_time"] += entry["duration"]
            op_type_status["min"] = min(op_type_status["min"], entry["duration"])
            op_type_status["max"] = max(op_type_status["max"], entry["duration"])
        for status in op_type_group.values():
            status["avg"] = status["total_time"] / status["count"]
        return op_type_group

    def calculate(self: any) -> None:
        """
        calculate hccl communication data and hccl op report data
        """
        with self._model as hccl_model:
            if not DBManager.check_tables_in_db(PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL),
                                                DBNameConstant.TABLE_HCCL_OP, DBNameConstant.TABLE_HCCL_TASK):
                logging.warning("The HCCL table does not exist, so there is no need to continue associating operators.")
                return

            iter_range: IterationRange = self.sample_config.get(StrConstant.PARAM_ITER_ID)
            hccl_tasks = hccl_model.get_hccl_task_data()
            hccl_ops = hccl_model.get_hccl_ops(model_id=iter_range.model_id, index_id=iter_range.iteration_id)
            communication_data = self._merge_hccl_ops_and_tasks(hccl_ops, hccl_tasks)

            if not communication_data:
                logging.error("communication data is empty")
                return

            self.update_bandwidth(communication_data)
            self.update_op_name_by_group_name(communication_data)
            is_hccl_op_type_valid = self._generate_hccl_op_info(communication_data)
            if is_hccl_op_type_valid:
                hccl_op_report_data = self._get_hccl_op_report_data(communication_data)
                self._create_report(hccl_op_report_data)
            else:
                logging.warning("No valid hccl op type exists, therefore not calculate hccl op report data")

    def save(self: any) -> None:
        with self._model as hccl_model:
            if not self._hccl_data:
                return
            hccl_model.rebuild_hccl_table()
            hccl_model.insert_data_to_db(DBNameConstant.TABLE_HCCL_SINGLE_DEVICE, self._hccl_data)
            if not self._hccl_op_report_data:
                return
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
        self._drop_table()
        self.calculate()
        self.save()

    def _drop_table(self):
        with self._model as hccl_model:
            hccl_model.drop_table(DBNameConstant.TABLE_HCCL_SINGLE_DEVICE)
            hccl_model.drop_table(DBNameConstant.TABLE_HCCL_OP_REPORT)

    def _judge_calculate_again(self):
        if not ProfilingScene().is_all_export():
            logging.info("In graph scene, to generate table %s and %s", DBNameConstant.TABLE_HCCL_SINGLE_DEVICE,
                         DBNameConstant.TABLE_HCCL_OP_REPORT)
            return True
        else:
            hccl_db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE)
            if DBManager.check_tables_in_db(hccl_db_path, DBNameConstant.TABLE_HCCL_SINGLE_DEVICE):
                logging.info("Found table %s in operator scene, no need to generate again",
                             DBNameConstant.TABLE_HCCL_SINGLE_DEVICE)
                return False
            logging.info("No table %s or %s found, to generate it", DBNameConstant.TABLE_HCCL_SINGLE_DEVICE,
                         DBNameConstant.TABLE_HCCL_OP_REPORT)
            return True

    def _generate_hccl_op_info(self, hccl_data: List[HcclTask]) -> bool:
        is_hccl_op_type_valid = False
        for data in hccl_data:
            self._hccl_data.append([data.model_id, data.index_id, data.op_name, data.iteration,
                                    data.hccl_name, data.group_name, data.first_timestamp, data.plane_id,
                                    data.timestamp, data.duration, data.is_dynamic,
                                    data.task_type, data.op_type, data.connection_id,
                                    data.is_master, data.stream_id, data.task_id, data.struct_type,
                                    data.duration_estimated, data.local_rank, data.remote_rank, data.transport_type,
                                    data.size, data.data_type, data.link_type, data.bandwidth, data.context_id,
                                    data.notify_id])
            if data.op_type != Constant.NA:
                is_hccl_op_type_valid = True
        return is_hccl_op_type_valid

    def _create_report(self, hccl_op_report_data) -> None:
        """
        calculate report data
        :return: None
        """
        task_data = hccl_op_report_data
        if not task_data:
            return
        hccl_op_total_time = self._cal_total(task_data)
        total_data = []
        for op_type in task_data:
            statistic_data = task_data.get(op_type, {})
            if not statistic_data:
                continue
            if hccl_op_total_time != 0:
                task_duration_ratio = round(float(statistic_data["total_time"] / hccl_op_total_time * 100),
                                            NumberConstant.DECIMAL_ACCURACY)
            else:
                task_duration_ratio = 0
            total_data.append(
                (
                    op_type,
                    statistic_data["count"],
                    round(float(statistic_data["total_time"]), NumberConstant.DECIMAL_ACCURACY),
                    round(float(statistic_data["min"]), NumberConstant.DECIMAL_ACCURACY),
                    round(float(statistic_data["avg"]), NumberConstant.DECIMAL_ACCURACY),
                    round(float(statistic_data["max"]), NumberConstant.DECIMAL_ACCURACY),
                    task_duration_ratio
                )
            )
        if total_data:
            self._hccl_op_report_data = sorted(total_data, key=lambda x: x[5], reverse=True)
        else:
            logging.warning("There is no hccl op report data. Maybe an error occurs during the calculation")

    def _merge_hccl_ops_and_tasks(self, hccl_ops: List[HcclOps], hccl_tasks: List[HcclTask]) -> List[HcclTask]:
        def update_task_desc_with_hccl_op(op_desc: HcclOps, task_desc: any, times_for_hccl_op: int) -> HcclTask:
            return task_desc.replace(op_name=op_desc.op_name,
                                     task_type=op_desc.task_type,
                                     op_type=op_desc.op_type,
                                     first_timestamp=op_desc.timestamp,
                                     iter_id=times_for_hccl_op,
                                     is_dynamic=op_desc.is_dynamic,
                                     model_id=op_desc.model_id,
                                     connection_id=op_desc.connection_id)

        if not hccl_ops or not hccl_tasks:
            logging.error("No Hccl ops or Hccl tasks found")
            return []

        idx = 0
        res = [None] * len(hccl_tasks)
        ops_queue = deque(hccl_ops)
        task_queue = deque(hccl_tasks)
        hccl_op_with_task = set()
        hccl_op_with_task_index = {}
        while ops_queue and task_queue:
            op = ops_queue.popleft()
            # check corner case: task time between last op end time and next op start time
            if task_queue and task_queue[0].host_timestamp < op.timestamp:
                logging.error('Hccl Task (context_id=%d, stream_id=%d, task_id=%d) timestamp not in Ops time range',
                              task_queue[0].context_id, task_queue[0].stream_id, task_queue[0].task_id)

            while task_queue and task_queue[0].host_timestamp <= (op.timestamp + op.duration):
                task = task_queue.popleft()
                key = f'{task.stream_id}-{task.task_id}-{task.context_id}'
                count = 1
                if key in hccl_op_with_task:
                    count = hccl_op_with_task_index.get(key, 1) + 1
                    hccl_op_with_task_index[key] = count
                hccl_op_with_task.add(key)
                task = update_task_desc_with_hccl_op(op, task, count)
                res[idx] = task
                idx += 1

        if ops_queue:
            logging.error('ops_queue is not empty (len=%d)', len(ops_queue))
        if task_queue:
            logging.error('task_queue is not empty (len=%d)', len(task_queue))

        return res[:idx]
