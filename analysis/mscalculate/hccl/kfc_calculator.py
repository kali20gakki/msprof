#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
import os
import logging
from collections import namedtuple

from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.db_name_constant import DBNameConstant
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from common_func.db_manager import DBManager
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_object import CustomizedNamedtupleFactory
from msmodel.task_time.ascend_task_model import AscendTaskModel
from msmodel.ge.ge_info_model import GeInfoViewModel
from msmodel.add_info.kfc_info_model import KfcInfoViewModel
from msmodel.add_info.mc2_comm_info_model import Mc2CommInfoViewModel
from msmodel.hccl.hccl_model import HCCLModel
from mscalculate.interface.icalculator import ICalculator
from mscalculate.flip.flip_calculator import FlipCalculator
from mscalculate.hccl.hccl_calculator import HcclCalculator


class KfcCalculator(ICalculator, MsMultiProcess):
    KFC_OP_DATA = CustomizedNamedtupleFactory.enhance_namedtuple(
        namedtuple("KfcOpData",
                   ["ascend_data", "group_name", "op_name", "first_timestamp", "iter_id", "op_type"]),
        {})
    BLACK_KFC_OP = ["NOTIFY_WAIT", "MEMCPY_ASYNC"]

    def __init__(self, file_list, sample_config):
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._kfc_op_data = []
        self._kfc_task_data = []
        self._comm_task = []

    @staticmethod
    def make_default_kfc_info() -> KfcInfoViewModel.KFC_HCCL_INFO_TYPE:
        default_kfc_info = KfcInfoViewModel.KFC_HCCL_INFO_TYPE(
            0, "N/A", 'N/A', 'N/A', 4294967295, 4294967295, -1, 0, -1, 4294967295, 'N/A', 'N/A', 'N/A', -1,
            'N/A', 'N/A', -1, 'N/A', 'N/A', 'N/A', 'N/A', 'N/A', -1, -1, 0, 0, 0, 0
        )
        return default_kfc_info

    def calculate(self: any) -> None:
        self.calculate_kfc_op()
        self.calculate_kfc_task()

    def save(self: any) -> None:
        if self._kfc_op_data:
            with HCCLModel(self._project_path, [DBNameConstant.TABLE_KFC_OP]) as model:
                model.flush(self._kfc_op_data, DBNameConstant.TABLE_KFC_OP)
        if self._kfc_task_data:
            with HCCLModel(self._project_path, [DBNameConstant.TABLE_KFC_TASK]) as model:
                model.flush(self._kfc_task_data, DBNameConstant.TABLE_KFC_TASK)

    def calculate_kfc_op(self: any) -> None:
        task_data = []
        conn, curs = DBManager.check_connect_db(self._project_path, DBNameConstant.DB_ASCEND_TASK)
        if conn and curs:
            DBManager.destroy_db_connect(conn, curs)
            with AscendTaskModel(self._project_path, [DBNameConstant.TABLE_ASCEND_TASK]) as model:
                task_data = model.get_ascend_task_data_without_unknown()
        with Mc2CommInfoViewModel(self._project_path, [DBNameConstant.TABLE_MC2_COMM_INFO]) as model:
            comm_info = model.get_kfc_stream(DBNameConstant.TABLE_MC2_COMM_INFO)
        kfc_stream_id = {}
        comm_stream_ids = []
        for info in comm_info:
            kfc_stream_id[info.aicpu_kfc_stream_id] = info.group_name
            comm_stream_ids += map(int, info.comm_stream_ids.split(","))
        kfc_op_data = []
        for data in task_data:
            if data.stream_id in kfc_stream_id and data.host_task_type not in self.BLACK_KFC_OP:
                # kfc大算子流
                kfc_op_data.append(self.KFC_OP_DATA(data, kfc_stream_id.get(data.stream_id, ""), "", 0, 1, ""))
                continue
            if data.stream_id in comm_stream_ids:
                self._comm_task.append(data)  # kfc小算子流
        with GeInfoViewModel(self._project_path, [DBNameConstant.TABLE_GE_TASK]) as model:
            ge_data = model.get_ge_info_by_device_id(DBNameConstant.TABLE_GE_TASK, InfoConfReader().get_device_id())
        node_name, host_timestamp = {}, {}
        for data in ge_data:
            if data.stream_id not in kfc_stream_id:
                continue
            node_key = "{0}-{1}-{2}-{3}".format(data.stream_id, data.task_id, data.context_id, data.batch_id)
            node_name[node_key] = [data.op_name, data.op_type]
            host_timestamp[node_key] = data.timestamp
        kfc_op_with_task = set()
        kfc_op_with_task_index = {}
        for i, op_data in enumerate(kfc_op_data):
            data = op_data.ascend_data
            iter_id = 1
            node_key = "{0}-{1}-{2}-{3}".format(data.stream_id, data.task_id, data.context_id, data.batch_id)
            if node_key in kfc_op_with_task:
                iter_id = kfc_op_with_task_index.get(node_key, 1) + 1
                kfc_op_with_task_index[node_key] = iter_id
            kfc_op_with_task.add(node_key)
            op_name, op_type = node_name.get(node_key, [None, None])
            if not op_name:
                logging.error("The kfc op name is None with task type %s", data.host_task_type)
                continue
            first_timestamp = host_timestamp.get(node_key, 0)
            kfc_op_data[i] = op_data.replace(op_name=op_name,
                                             first_timestamp=first_timestamp, iter_id=iter_id, op_type=op_type)
        HcclCalculator.update_op_name_by_group_name(kfc_op_data)
        self._kfc_op_data = [[data.ascend_data.model_id, data.ascend_data.index_id, data.op_name,
                              data.ascend_data.start_time, data.ascend_data.duration, data.group_name,
                              data.ascend_data.connection_id, data.op_type] for data in kfc_op_data]

    def calculate_kfc_task(self: any) -> None:
        # 由于mc2没有上报翻转flip信息，所以不呈现hccl小算子
        kfc_info_data = [None] * len(self._comm_task)
        for idx, data in enumerate(self._comm_task):
            kfc_info_data[idx] = self.make_default_kfc_info()
            kfc_info_data[idx] = kfc_info_data[idx].replace(start_time=data.start_time, duration=data.duration,
                                                            stream_id=data.stream_id, task_id=data.task_id,
                                                            context_id=data.context_id, batch_id=data.batch_id)
        idx = 0
        match_num = 0
        self._kfc_task_data = [None] * len(kfc_info_data)
        for kfc_op in self._kfc_op_data:
            # kfc_op[3]: kfc op start time
            while idx < len(kfc_info_data) and kfc_info_data[idx].start_time < kfc_op[3]:
                idx += 1
            # kfc_op[3]: kfc op start time; kfc_op[4]: kfc op duration
            while idx < len(kfc_info_data) and kfc_info_data[idx].start_time <= kfc_op[3] + kfc_op[4]:
                data = kfc_info_data[idx]
                group_name = data.group_name if data.group_name != "N/A" else kfc_op[5]
                self._kfc_task_data[match_num] = [
                    # kfc op的前5个字段: model_id, index_id, op_name, start_time, duration
                    *kfc_op[:5], 0, data.op_name, group_name, data.plane_id, data.start_time, data.duration,
                    data.stream_id, data.task_id, data.duration_estimated, data.local_rank, data.remote_rank,
                    data.transport_type, data.size, data.data_type, data.link_type, data.bandwidth, data.context_id,
                    data.notify_id, data.batch_id, data.rdma_type
                ]
                match_num += 1
                idx += 1
        if idx > match_num:
            logging.warning("The kfc info is mismatched with kfc op, mismatch num is %d", idx - match_num)
        self._kfc_task_data = self._kfc_task_data[:match_num]

    def ms_run(self: any) -> None:
        """
        entry
        :return: None`
        """
        if not os.path.exists(PathManager.get_db_path(self._project_path, DBNameConstant.DB_MC2_COMM_INFO)) \
                or not self._judge_calculate_again():
            return
        self._drop_table()
        self.calculate()
        self.save()

    def _drop_table(self):
        with HCCLModel(self._project_path, [DBNameConstant.TABLE_KFC_OP, DBNameConstant.TABLE_KFC_TASK]) as model:
            model.drop_table(DBNameConstant.TABLE_KFC_OP)
            model.drop_table(DBNameConstant.TABLE_KFC_TASK)

    def _judge_calculate_again(self):
        if not ProfilingScene().is_all_export():
            logging.info("In graph scene, to generate table %s", DBNameConstant.TABLE_KFC_OP)
            return True
        else:
            hccl_db_path = PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE)
            if DBManager.check_tables_in_db(hccl_db_path, DBNameConstant.TABLE_KFC_OP):
                logging.info("Found table %s in operator scene, no need to generate again",
                             DBNameConstant.TABLE_KFC_OP)
                return False
            logging.info("No table %s found, to generate it", DBNameConstant.TABLE_KFC_OP)
            return True
