#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import os

from common_func.config_mgr import ConfigMgr
from common_func.data_check_manager import DataCheckManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_common import analyze_collect_data
from common_func.path_manager import PathManager
from mscalculate.interface.icalculator import ICalculator
from msmodel.acl.acl_model import AclModel
from msmodel.msproftx.msproftx_model import MsprofTxModel
from msmodel.sync_acl_npu.sync_acl_npu_model import SyncAclNpuModel
from profiling_bean.prof_enum.data_tag import DataTag


class TorchAclRelationCalculator(ICalculator, MsMultiProcess):
    MSPROF_HOST_DIR = "host"

    def __init__(self, file_list: dict, sample_config: dict):
        super().__init__(sample_config)
        self._result_dir = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._torch_op_data = []
        self._mark_data = []
        self._acl_op_execute_data = []
        self._acl_op_compile_data = []
        self._acl_op_execute_dict = {}
        self._matched_torch_acl_data = []

    def ms_run(self):
        self._load_data()
        if not self._torch_op_data or not self._mark_data or not self._acl_op_execute_data:
            return
        self.calculate()
        self.save()

    def calculate(self: any) -> None:
        self._compute_compile_time()
        self._match_acl_op_execute()
        self._match_torch_op()

    def save(self: any) -> None:
        if self._matched_torch_acl_data:
            sync_acl_npu_model = SyncAclNpuModel(self._result_dir, [DBNameConstant.TABLE_TORCH_TO_ACL])
            sync_acl_npu_model.clear()
            with sync_acl_npu_model:
                sync_acl_npu_model.flush(DBNameConstant.TABLE_TORCH_TO_ACL, self._matched_torch_acl_data)

    def _load_data(self):
        host_dir = os.path.realpath(os.path.join(self._result_dir, "..", self.MSPROF_HOST_DIR))
        if not DataCheckManager.contain_info_json_data(host_dir):
            return
        if not FileManager.is_analyzed_data(host_dir):
            InfoConfReader().load_info(host_dir)
            analyze_collect_data(host_dir, ConfigMgr.read_sample_config(host_dir))
        if not os.path.exists(PathManager.get_db_path(host_dir, DBNameConstant.DB_MSPROFTX)) or not os.path.exists(
                PathManager.get_db_path(self._result_dir, DBNameConstant.DB_ACL_MODULE)):
            return
        with MsprofTxModel(host_dir, DBNameConstant.DB_MSPROFTX, [DBNameConstant.TABLE_MSPROFTX]) as model:
            self._torch_op_data = model.get_torch_op_data()
            self._mark_data = model.get_mark_data()
        with AclModel({StrConstant.PARAM_RESULT_DIR: self._result_dir}) as model:
            self._acl_op_execute_data = model.get_acl_op_execute_data()
            self._acl_op_compile_data = model.get_acl_op_compile_data()

    def _compute_compile_time(self):
        start_index = 0
        end_index = len(self._acl_op_compile_data)
        for acl_op_exe in self._acl_op_execute_data:
            compile_time = 0
            for compile_index in range(start_index, end_index):
                if self._acl_op_compile_data[compile_index].start_time < acl_op_exe.start_time:
                    continue
                if self._acl_op_compile_data[compile_index].start_time > acl_op_exe.end_time:
                    start_index = compile_index
                    break
                op_compile_data = self._acl_op_compile_data[compile_index]
                compile_time += (op_compile_data.end_time - op_compile_data.start_time)
            setattr(acl_op_exe, "compile_time", compile_time)

    def _match_acl_op_execute(self):
        for mark_data in self._mark_data:
            matched_acl_data = None
            while self._acl_op_execute_data:
                if mark_data.start_time >= self._acl_op_execute_data[-1].start_time:
                    break
                matched_acl_data = self._acl_op_execute_data.pop()
            acl_start_time = matched_acl_data.start_time if matched_acl_data else None
            acl_end_time = matched_acl_data.end_time if matched_acl_data else None
            acl_tid = matched_acl_data.thread_id if matched_acl_data else None
            acl_compile_time = matched_acl_data.compile_time if matched_acl_data else None
            setattr(mark_data, "acl_start_time", acl_start_time)
            setattr(mark_data, "acl_end_time", acl_end_time)
            setattr(mark_data, "acl_tid", acl_tid)
            setattr(mark_data, "acl_compile_time", acl_compile_time)
            self._acl_op_execute_dict.setdefault(mark_data.message, []).append(mark_data)

    def _match_torch_op(self):
        for torch_op in self._torch_op_data:
            acl_op_execute_list = self._acl_op_execute_dict.get(torch_op.message, [])
            if acl_op_execute_list:
                acl_op_execute = acl_op_execute_list.pop()
                self._matched_torch_acl_data.append(
                    [acl_op_execute.message, acl_op_execute.start_time, acl_op_execute.acl_start_time,
                     acl_op_execute.acl_end_time, acl_op_execute.acl_tid, acl_op_execute.acl_compile_time,
                     torch_op.start_time, torch_op.tid, torch_op.pid])
