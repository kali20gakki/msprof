#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
import os

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.path_manager import PathManager
from msmodel.sync_acl_npu.sync_acl_npu_model import SyncAclNpuViewModel
from profiling_bean.prof_enum.export_data_type import ExportDataType


class TorchToAclNpu:
    ACL_OP_NAME = ['aclopCompileAndExecute', 'aclopCompileAndExecuteV2']
    MSPROF_HOST_DIR = "host"
    TORCH_PID = ExportDataType.MSPROF_TX.value
    NPU_PID = f"{ExportDataType.TASK_TIME.value}_0"

    def __init__(self, result_dir: str):
        self._result_dir = result_dir
        self._torch_acl_data_list = []
        self._torch_npu_data_list = []

    def add_connect_line(self: any, json_data: list):
        self._load_data()
        self._add_torch_to_acl_line(json_data)
        self._add_torch_to_npu_line(json_data)

    def _load_data(self):
        if not os.path.exists(PathManager.get_db_path(self._result_dir, DBNameConstant.DB_SYNC_ACL_NPU)):
            return
        with SyncAclNpuViewModel(self._result_dir,
                                 [DBNameConstant.TABLE_TORCH_TO_ACL, DBNameConstant.TABLE_TORCH_TO_NPU]) as model:
            if model.check_table():
                self._torch_acl_data_list = model.get_torch_acl_relation_data()
                self._torch_npu_data_list = model.get_torch_npu_relation_data()

    def _add_torch_to_acl_line(self: any, json_data: list):
        if not self._torch_acl_data_list:
            return
        for torch_acl_data in self._torch_acl_data_list:
            start_point = {
                'name': 'torch_to_acl', 'ph': 's', 'id': torch_acl_data.acl_start_time / DBManager.NSTOUS,
                'pid': f'{self.TORCH_PID}_{torch_acl_data.torch_op_pid}', 'tid': torch_acl_data.torch_op_tid,
                'ts': torch_acl_data.torch_op_start_time / DBManager.NSTOUS, 'cat': StrConstant.ASYNC_ACL_NPU
            }
            json_data.append(start_point)
        end_point_list = []
        for data in json_data:
            if data.get('name', '') in self.ACL_OP_NAME:
                end_point = {
                    'name': 'torch_to_acl', 'ph': 'f', 'id': data.get('ts'), 'pid': data.get('pid'),
                    'tid': data.get('tid'), 'ts': data.get('ts'), 'bp': 'e', 'cat': StrConstant.ASYNC_ACL_NPU
                }
                end_point_list.append(end_point)
        json_data.extend(end_point_list)

    def _add_torch_to_npu_line(self: any, json_data: list):
        if not self._torch_npu_data_list:
            return
        for torch_npu_data in self._torch_npu_data_list:
            start_point = {
                'name': 'torch_to_npu', 'ph': 's',
                'id': f'{torch_npu_data.stream_id}_{torch_npu_data.task_id}_{torch_npu_data.batch_id}',
                'pid': f'{self.TORCH_PID}_{torch_npu_data.torch_op_pid}', 'tid': torch_npu_data.torch_op_tid,
                'ts': torch_npu_data.torch_op_start_time / DBManager.NSTOUS, 'cat': StrConstant.ASYNC_NPU
            }
            json_data.append(start_point)
        end_point_list = []
        for data in json_data:
            if data.get('pid', '') == self.NPU_PID:
                args = data.get('args', {})
                stream_id = args.get('Stream Id')
                task_id = args.get('Task Id')
                batch_id = args.get('Batch Id')
                if not stream_id or not task_id:
                    continue
                end_point = {
                    'name': 'torch_to_npu', 'ph': 'f', 'id': f'{stream_id}_{task_id}_{batch_id}',
                    'pid': data.get('pid'), 'tid': data.get('tid'), 'ts': data.get('ts'), 'bp': 'e',
                    'cat': StrConstant.ASYNC_NPU
                }
                end_point_list.append(end_point)
        json_data.extend(end_point_list)
