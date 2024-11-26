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
from common_func.hccl_info_common import DeviceHcclOpSource
from msmodel.task_time.ascend_task_model import AscendTaskViewModel
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
                   ["ascend_data", "group_name", "op_name", "first_timestamp", "iter_id", "op_type",
                    "relay", "retry", "data_type", "alg_type", "count", "rank_size", "source"]),
        {})
    BLACK_KFC_OP = ["NOTIFY_WAIT", "MEMCPY_ASYNC"]
    INVALID_GROUP = "N/A"
    MC2_MAIN_STREAM_TASK_TYPE = "C_CORE_SQE"

    def __init__(self, file_list, sample_config):
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._kfc_op_data = {}
        self._kfc_task_data = {}
        self._kfc_small_task = {}
        self._kfc_stream_id = {}
        self._plane_id = {}
        self._main_stream = set()

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
            kfc_op_data = []
            for data in self._kfc_op_data.values():
                kfc_op_data.extend(data)
            with HCCLModel(self._project_path, [DBNameConstant.TABLE_KFC_OP]) as model:
                model.flush(kfc_op_data, DBNameConstant.TABLE_KFC_OP)
        if self._kfc_task_data:
            kfc_task_data = []
            for data in self._kfc_task_data.values():
                kfc_task_data.extend(data)
            with HCCLModel(self._project_path, [DBNameConstant.TABLE_KFC_TASK]) as model:
                model.flush(kfc_task_data, DBNameConstant.TABLE_KFC_TASK)

    def set_main_stream(self: any, comm_task_data: list) -> None:
        for data in comm_task_data:
            if data.device_task_type == self.MC2_MAIN_STREAM_TASK_TYPE:
                self._main_stream.add(data.stream_id)

    def get_kfc_op_data(self: any) -> dict:
        with Mc2CommInfoViewModel(self._project_path, [DBNameConstant.TABLE_MC2_COMM_INFO]) as model:
            comm_info = model.get_kfc_stream(DBNameConstant.TABLE_MC2_COMM_INFO)
        self._kfc_stream_id = {}
        comm_stream_ids = {}
        for info in comm_info:
            self._kfc_stream_id[info.aicpu_kfc_stream_id] = info.group_name
            comm_stream_list = []
            try:
                comm_stream_list = list(map(int, info.comm_stream_ids.split(",")))
            except Exception:
                logging.error("The comm_stream_ids is not number")
            for stream_id in comm_stream_list:
                comm_stream_ids[stream_id] = info.group_name
        kfc_op_task_data = []
        kfc_comm_task_data = []
        conn, curs = DBManager.check_connect_db(self._project_path, DBNameConstant.DB_ASCEND_TASK)
        if conn and curs:
            DBManager.destroy_db_connect(conn, curs)
            with AscendTaskViewModel(self._project_path, [DBNameConstant.TABLE_ASCEND_TASK]) as model:
                kfc_op_task_data = model.get_ascend_task_data_with_op_name_pattern_and_stream_id(
                    InfoConfReader().get_device_id(),
                    "AicpuKernel",
                    tuple(self._kfc_stream_id.keys())
                )
                kfc_comm_task_data = model.get_ascend_task_data_with_stream_id(tuple(comm_stream_ids.keys()))
        kfc_op_data = {}
        for data in kfc_op_task_data:
            # kfc大算子流
            if data.host_task_type in self.BLACK_KFC_OP:
                continue
            group_name = self._kfc_stream_id.get(data.stream_id, "N/A")
            kfc_op_data.setdefault(data.stream_id, []).append(
                self.KFC_OP_DATA(data, group_name, "N/A", 0, 1, "N/A",
                                 -1, -1, "N/A", "N/A", -1, -1, DeviceHcclOpSource.INVALID.value)
            )
        task_stream_ids = {}
        self.set_main_stream(kfc_comm_task_data)
        for data in kfc_comm_task_data:
            # kfc小算子流
            group_name = comm_stream_ids.get(data.stream_id, self.INVALID_GROUP)
            self._kfc_small_task.setdefault(group_name, []).append(data)
            task_stream_ids.setdefault(group_name, set()).add(data.stream_id)
        for group_name, stream_ids in task_stream_ids.items():
            stream_ids = list(stream_ids)
            stream_ids.sort()
            for i, stream_id in enumerate(stream_ids):
                self._plane_id.setdefault(group_name, {})[stream_id] = i
        return kfc_op_data

    def get_host_task_info(self: any, kfc_op_data) -> tuple:
        with GeInfoViewModel(self._project_path, [DBNameConstant.TABLE_GE_TASK]) as model:
            ge_data = model.get_ge_info_by_device_id(DBNameConstant.TABLE_GE_TASK, InfoConfReader().get_device_id())
        node_name = {}
        host_timestamp = {}
        for data in ge_data:
            if data.stream_id not in kfc_op_data:
                continue
            node_key = "{0}-{1}-{2}-{3}".format(data.stream_id, data.task_id, data.context_id, data.batch_id)
            node_name[node_key] = [data.op_name, data.op_type]
            host_timestamp[node_key] = data.timestamp
        return node_name, host_timestamp

    def get_hccl_op_info_data(self: any) -> dict:
        with KfcInfoViewModel(self._project_path,
                              [DBNameConstant.TABLE_DEVICE_HCCL_OP_INFO]) as kfc_info_model:
            hccl_op_info = kfc_info_model.get_hccl_op_info_data()
        hccl_op_info_dict = {}
        for hccl_op in hccl_op_info:
            hccl_op_info_dict.setdefault(hccl_op.stream_id, []).append(hccl_op)
        return hccl_op_info_dict

    def calculate_kfc_op(self: any) -> None:
        kfc_op_data_by_stream_id = self.get_kfc_op_data()
        node_name, host_timestamp = self.get_host_task_info(kfc_op_data_by_stream_id)
        hccl_op_info_data = self.get_hccl_op_info_data()
        for stream_id, kfc_op_data in kfc_op_data_by_stream_id.items():
            idx = 0
            hccl_op_info = hccl_op_info_data.get(stream_id, [])
            kfc_op_with_task = set()
            kfc_op_with_task_index = {}
            for i, op_data in enumerate(kfc_op_data):
                group_name = op_data.group_name
                while idx < len(hccl_op_info) and hccl_op_info[idx].timestamp < op_data.ascend_data.start_time:
                    idx += 1
                curr_hccl_op_info = None
                if idx < len(hccl_op_info) and \
                        op_data.ascend_data.start_time <= hccl_op_info[idx].timestamp <= \
                        op_data.ascend_data.start_time + op_data.ascend_data.duration:
                    curr_hccl_op_info = hccl_op_info[idx]
                    group_name = curr_hccl_op_info.group_name
                    idx += 1
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
                if curr_hccl_op_info:
                    kfc_op_data[i] = op_data.replace(op_name=op_name, first_timestamp=first_timestamp,
                                                     iter_id=iter_id, op_type=op_type, group_name=group_name,
                                                     relay=curr_hccl_op_info.relay, retry=curr_hccl_op_info.retry,
                                                     data_type=curr_hccl_op_info.data_type,
                                                     alg_type=curr_hccl_op_info.alg_type, count=curr_hccl_op_info.count,
                                                     rank_size=curr_hccl_op_info.rank_size,
                                                     source=curr_hccl_op_info.source)
                else:
                    kfc_op_data[i] = op_data.replace(op_name=op_name, group_name=group_name,
                                                     first_timestamp=first_timestamp, iter_id=iter_id, op_type=op_type)
            HcclCalculator.update_op_name_by_group_name(kfc_op_data)
            for data in kfc_op_data:
                self._kfc_op_data.setdefault(data.group_name, []).append(
                    [data.ascend_data.model_id, data.ascend_data.index_id, data.op_name,
                     data.ascend_data.start_time, data.ascend_data.duration, data.group_name,
                     data.ascend_data.connection_id, data.op_type, data.relay, data.retry,
                     data.data_type, data.alg_type, data.count, data.rank_size, data.source])

    def process_kfc_info_data(self: any, task_info: list, kfc_info_data: list) -> list:
        if not kfc_info_data:
            kfc_info_data = [None] * len(task_info)
            for idx, data in enumerate(task_info):
                kfc_info_data[idx] = self.make_default_kfc_info()
                kfc_info_data[idx] = kfc_info_data[idx].replace(start_time=data.start_time, duration=data.duration,
                                                                stream_id=data.stream_id, task_id=data.task_id,
                                                                context_id=data.context_id, batch_id=data.batch_id)
            return kfc_info_data
        task_time = {}
        for data in task_info:
            node_key = "{0}-{1}-{2}-{3}".format(data.stream_id, data.task_id, data.context_id, data.batch_id)
            task_time[node_key] = [data.start_time, data.duration]
        for idx, data in enumerate(kfc_info_data):
            node_key = "{0}-{1}-{2}-{3}".format(data.stream_id, data.task_id, data.context_id, data.batch_id)
            node_time = task_time.get(node_key, [0, 0])
            kfc_info_data[idx] = data.replace(start_time=node_time[0], duration=node_time[1])
        HcclCalculator.update_bandwidth(kfc_info_data)
        return kfc_info_data

    def calculate_kfc_task(self: any) -> None:
        # 由于mc2没有上报翻转flip信息，所以不呈现hccl小算子
        kfc_info = []
        kfc_info_by_group_name = {}
        for data in kfc_info:
            kfc_info_by_group_name.setdefault(data.group_name, []).append(data)
        for group_name, task_info in self._kfc_small_task.items():
            kfc_info_data = kfc_info_by_group_name.get(group_name, [])
            kfc_info_data = self.process_kfc_info_data(task_info, kfc_info_data)
            idx = 0
            match_num = 0
            kfc_op_data = self._kfc_op_data.get(group_name, [])
            self._kfc_task_data[group_name] = [None] * len(kfc_info_data)
            for kfc_op in kfc_op_data:
                # kfc_op[3]: kfc op start time
                while idx < len(kfc_info_data) and kfc_info_data[idx].start_time < kfc_op[3]:
                    idx += 1
                # kfc_op[3]: kfc op start time; kfc_op[4]: kfc op duration
                while idx < len(kfc_info_data) and kfc_info_data[idx].start_time <= kfc_op[3] + kfc_op[4]:
                    data = kfc_info_data[idx]
                    plane_id = data.plane_id if data.plane_id == -1 \
                        else self._plane_id.get(group_name, {}).get(data.stream_id, -1)
                    is_master = 1 if data.stream_id in self._main_stream else 0
                    self._kfc_task_data[group_name][match_num] = [
                        *kfc_op[:5], 0, data.op_name, group_name, plane_id, data.start_time, data.duration, is_master,
                        data.stream_id, data.task_id, data.duration_estimated, data.local_rank, data.remote_rank,
                        data.transport_type, data.size, data.data_type, data.link_type, data.bandwidth, data.context_id,
                        data.notify_id, data.batch_id, data.rdma_type
                    ]
                    match_num += 1
                    idx += 1
            if idx > match_num:
                logging.warning("The kfc info is mismatched with kfc op, mismatch num is %d", idx - match_num)
            self._kfc_task_data[group_name] = self._kfc_task_data[group_name][:match_num]

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
