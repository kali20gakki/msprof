#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
import os
import logging

from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.db_name_constant import DBNameConstant
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from common_func.db_manager import DBManager
from common_func.info_conf_reader import InfoConfReader
from msmodel.task_time.ascend_task_model import AscendTaskModel
from msmodel.ge.ge_info_model import GeInfoViewModel
from msmodel.add_info.kfc_info_model import KfcInfoModel
from msmodel.add_info.kfc_info_model import KfcInfoViewModel
from msmodel.add_info.mc2_comm_info_model import Mc2CommInfoViewModel
from mscalculate.interface.icalculator import ICalculator
from mscalculate.flip.flip_calculator import FlipCalculator
from mscalculate.hccl.hccl_calculator import HcclCalculator


class KfcCalculator(ICalculator, MsMultiProcess):
    def __init__(self, file_list, sample_config):
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._kfc_op_data = []
        self._kfc_task_data = []
        self._comm_task = []

    def calculate(self: any) -> None:
        self.calculate_kfc_op()
        self.calculate_kfc_task()

    def save(self: any) -> None:
        if self._kfc_op_data:
            with KfcInfoModel(self._project_path, [DBNameConstant.TABLE_KFC_OP]) as model:
                model.flush(self._kfc_op_data, DBNameConstant.TABLE_KFC_OP)
        if self._kfc_task_data:
            with KfcInfoModel(self._project_path, [DBNameConstant.TABLE_KFC_TASK]) as model:
                model.flush(self._kfc_task_data, DBNameConstant.TABLE_KFC_TASK)

    def calculate_kfc_op(self: any) -> None:
        with AscendTaskModel(self._project_path, [DBNameConstant.TABLE_ASCEND_TASK]) as model:
            task_data = model.get_ascend_task_data_without_unknown()
        with Mc2CommInfoViewModel(self._project_path,
                                  [DBNameConstant.TABLE_MC2_COMM_INFO]) as model:
            comm_info = model.get_kfc_stream(DBNameConstant.TABLE_MC2_COMM_INFO)
        kfc_stream_id = {}
        comm_stream_ids = []
        for info in comm_info:
            kfc_stream_id[info.aicpu_kfc_stream_id] = info.group_name
            comm_stream_ids += map(int, info.comm_stream_ids.split(","))
        kfc_op_data = []
        for data in task_data:
            if data.stream_id in kfc_stream_id:
                setattr(data, 'group_name', kfc_stream_id.get(data.stream_id, ""))
                kfc_op_data.append(data)  # kfc大算子流
                continue
            if data.stream_id in comm_stream_ids:
                self._comm_task.append(data)  # kfc小算子流
        with GeInfoViewModel(self._project_path, [DBNameConstant.TABLE_GE_TASK]) as model:
            ge_data = model.get_ge_info_by_device_id(DBNameConstant.TABLE_GE_TASK, InfoConfReader().get_device_id())
        node_name = {}
        for data in ge_data:
            if data.stream_id not in kfc_stream_id:
                continue
            node_key = "{0}-{1}-{2}-{3}".format(data.stream_id, data.task_id, data.context_id, data.batch_id)
            node_name[node_key] = data.op_name
        for data in kfc_op_data:
            node_key = "{0}-{1}-{2}-{3}".format(data.stream_id, data.task_id, data.context_id, data.batch_id)
            setattr(data, 'op_name', node_name.get(node_key, data.host_task_type))
        self._kfc_op_data = [
            [data.model_id, data.index_id, data.op_name, data.start_time,
             data.duration, data.group_name] for data in kfc_op_data
        ]

    def calculate_kfc_task(self: any) -> None:
        with KfcInfoViewModel(self._project_path,
                              [DBNameConstant.TABLE_KFC_INFO]) as kfc_info_model:
            kfc_info_data = kfc_info_model.get_kfc_info_data()
        if not kfc_info_data:
            return
        kfc_info_data = FlipCalculator.set_device_batch_id(kfc_info_data, self._project_path)
        task_time = {}
        for data in self._comm_task:
            node_key = "{0}-{1}-{2}-{3}".format(data.stream_id, data.task_id, data.context_id, data.batch_id)
            task_time[node_key] = [data.start_time, data.duration]
        for idx, data in enumerate(kfc_info_data):
            node_key = "{0}-{1}-{2}-{3}".format(data.stream_id, data.task_id, data.context_id, data.batch_id)
            node_time = task_time.get(node_key, [0, 0])
            kfc_info_data[idx] = data.replace(start_time=node_time[0], duration=node_time[1])
        HcclCalculator.update_bandwidth(kfc_info_data)
        self._kfc_task_data = [
            [data.op_name, data.group_name, data.plane_id, data.start_time, data.duration,
             data.stream_id, data.task_id, data.duration_estimated, data.local_rank, data.remote_rank,
             data.transport_type, data.size, data.data_type, data.link_type, data.bandwidth, data.context_id,
             data.notify_id, data.batch_id, data.rdma_type] for data in kfc_info_data
        ]

    def ms_run(self: any) -> None:
        """
        entry
        :return: None`
        """
        if not os.path.exists(PathManager.get_db_path(self._project_path, DBNameConstant.DB_KFC_INFO)) \
                or not self._judge_calculate_again():
            return
        self._drop_table()
        self.calculate()
        self.save()

    def _drop_table(self):
        with KfcInfoViewModel(self._project_path,
                              [DBNameConstant.TABLE_KFC_OP, DBNameConstant.TABLE_KFC_TASK]) as model:
            model.drop_table(DBNameConstant.TABLE_KFC_OP)
            model.drop_table(DBNameConstant.TABLE_KFC_TASK)

    def _judge_calculate_again(self):
        if not ProfilingScene().is_all_export():
            logging.info("In graph scene, to generate table %s", DBNameConstant.TABLE_KFC_OP)
            return True
        else:
            kfc_db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_KFC_INFO)
            if DBManager.check_tables_in_db(kfc_db_path, DBNameConstant.TABLE_KFC_OP):
                logging.info("Found table %s in operator scene, no need to generate again",
                             DBNameConstant.TABLE_KFC_TASK)
                return False
            logging.info("No table %s found, to generate it", DBNameConstant.TABLE_HCCL_TASK_SINGLE_DEVICE)
            return True
