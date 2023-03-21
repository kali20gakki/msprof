#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import os

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from mscalculate.interface.icalculator import ICalculator
from msmodel.interface.view_model import ViewModel
from msmodel.sync_acl_npu.sync_acl_npu_model import SyncAclNpuModel, SyncAclNpuViewModel
from profiling_bean.db_dto.ge_task_dto import GeTaskDto


class TorchNpuRelationCalculator(ICalculator, MsMultiProcess):
    MSPROF_HOST_DIR = "host"
    EXCEPT_OP_TYPE = ("TransData", "Transpose", "Cast", "DynamicAtomicAddrClean")

    def __init__(self, file_list: dict, sample_config: dict):
        super().__init__(sample_config)
        self._result_dir = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self.torch_npu_relation_data = []

    @classmethod
    def _check_full_op_name(cls: any, torch_acl: object, npu_op_list: list) -> list:
        result = set()
        for npu_op_index, npu_op in enumerate(npu_op_list):
            if npu_op.op_name == torch_acl.op_name:
                result.add(npu_op_index)
        return result

    @classmethod
    def _check_part_op_name(cls: any, torch_acl: object, npu_op_list: list) -> list:
        result = set()
        for npu_op_index, npu_op in enumerate(npu_op_list):
            if npu_op.op_name.lower().find(torch_acl.op_name.lower()) >= 0 and npu_op.op_type not in cls.EXCEPT_OP_TYPE:
                result.add(npu_op_index)
        return result

    @classmethod
    def _except_trans_op_name(cls: any, npu_op_list: list) -> list:
        result = set()
        for npu_op_index, npu_op in enumerate(npu_op_list):
            if npu_op.op_type not in cls.EXCEPT_OP_TYPE:
                result.add(npu_op_index)
        return result

    def ms_run(self: any) -> None:
        if self._result_dir.split("\\")[-1] == self.MSPROF_HOST_DIR:
            return
        self.calculate()
        self.save()

    def calculate(self: any) -> None:
        if not os.path.exists(
                PathManager.get_db_path(self._result_dir, DBNameConstant.DB_SYNC_ACL_NPU)) or not os.path.exists(
            PathManager.get_db_path(self._result_dir, DBNameConstant.DB_GE_INFO)):
            return
        torch_acl_data, ge_data = [], []
        with SyncAclNpuViewModel(self._result_dir, [DBNameConstant.TABLE_TORCH_TO_ACL]) as model:
            if model.check_table():
                torch_acl_data = model.get_torch_acl_relation_data()
        with ViewModel(self._result_dir, DBNameConstant.DB_GE_INFO, [DBNameConstant.TABLE_GE_TASK]) as model:
            if model.check_table():
                sql = f"select stream_id, task_id, batch_id, context_id, timestamp, op_name, op_type " \
                      f"from {DBNameConstant.TABLE_GE_TASK} order by timestamp desc"
                ge_data = DBManager.fetch_all_data(model.cur, sql, dto_class=GeTaskDto)
        if not torch_acl_data or not ge_data:
            return
        for torch_acl in torch_acl_data:
            npu_op_list = []
            while ge_data:
                if ge_data[-1].timestamp < torch_acl.acl_start_time:
                    ge_data.pop()
                    continue
                if ge_data[-1].timestamp > torch_acl.acl_end_time:
                    break
                npu_op_list.append(ge_data.pop())
            if not npu_op_list:
                continue
            self._torch_match_npu(torch_acl, npu_op_list)

    def save(self: any) -> None:
        if self.torch_npu_relation_data:
            with SyncAclNpuModel(self._result_dir, [DBNameConstant.TABLE_TORCH_TO_NPU]) as _model:
                _model.flush(DBNameConstant.TABLE_TORCH_TO_NPU, self.torch_npu_relation_data)

    def _torch_match_npu(self: any, torch_acl: object, npu_op_list: list):
        if len(npu_op_list) == 1:
            matched_index_list = {0}
        else:
            matched_index_list = self._check_full_op_name(torch_acl, npu_op_list)
            if not matched_index_list:
                matched_index_list = self._check_part_op_name(torch_acl, npu_op_list)
            if not matched_index_list:
                matched_index_list = self._except_trans_op_name(npu_op_list)
        for npu_op_index, npu_op in enumerate(npu_op_list):
            is_main_kernel = 1 if npu_op_index in matched_index_list else 0
            self.torch_npu_relation_data.append(
                [torch_acl.torch_op_start_time, torch_acl.torch_op_tid, torch_acl.torch_op_pid, torch_acl.op_name,
                 torch_acl.acl_start_time, torch_acl.acl_end_time, torch_acl.acl_tid, torch_acl.acl_compile_time,
                 npu_op.stream_id, npu_op.task_id, npu_op.batch_id, npu_op.context_id, npu_op.op_name, npu_op.op_type,
                 is_main_kernel
                 ])
