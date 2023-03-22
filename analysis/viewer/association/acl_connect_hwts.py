#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

import os

from common_func.ai_stack_data_check_manager import AiStackDataCheckManager
from common_func.db_name_constant import DBNameConstant
from common_func.msvp_common import path_check
from common_func.path_manager import PathManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.interface.view_model import ViewModel


class AclToHwts:
    MODULE_ACL = 'acl'
    MODULE_TASK_TIME = 'task_time'
    ACL_OP_NAME = (
        f'{TraceViewHeaderConstant.PROCESS_ACL}@aclopExecute',
        f'{TraceViewHeaderConstant.PROCESS_ACL}@aclopExecuteV2',
        f'{TraceViewHeaderConstant.PROCESS_ACL}@aclopCompileAndExecute',
        f'{TraceViewHeaderConstant.PROCESS_ACL}@aclopCompileAndExecuteV2',
    )
    ACL_OP_TYPE = 'ACL_OP'

    def __init__(self: any, result_dir: str) -> None:
        self._result_dir = result_dir

    @staticmethod
    def judge_dict_start(data_dict: dict) -> bool:
        """
        judge acl dict start point
        :return: bool
        """
        acl_args = data_dict.get('args', '')
        if data_dict.get('name', '') not in AclToHwts.ACL_OP_NAME or not acl_args:
            return False
        if acl_args.get('Mode', '') != AclToHwts.ACL_OP_TYPE:
            return False
        return True

    @staticmethod
    def add_hwts_connect_end(json_list: list) -> None:
        """
        add connect end point
        :return: None
        """
        TraceViewManager.add_connect_end_point(json_list)

    def add_connect_line(self: any, json_list: list, data_type: str) -> None:
        """
        add connect line with task time
        :param json_list: json list
        :param data_type: data_type
        """
        if AiStackDataCheckManager.contain_acl_data(
                self._result_dir) and AiStackDataCheckManager.contain_core_cpu_reduce_data(self._result_dir):
            if data_type == self.MODULE_ACL:
                self.add_acl_connect_start(json_list)
            if data_type == self.MODULE_TASK_TIME:
                self.add_hwts_connect_end(json_list)

    def add_acl_connect_start(self: any, json_list: list) -> None:
        """
        add connect start point
        :return: None
        """
        ge_data_list = self.get_ge_data()
        if not isinstance(json_list, list):
            return
        for data_dict in json_list:
            if not AclToHwts.judge_dict_start(data_dict):
                continue
            connect_list = TraceViewManager.add_connect_start_point(data_dict, ge_data_list)
            if connect_list:
                json_list.extend(connect_list)

    def get_ge_data(self: any) -> list:
        """
        get ge data
        :return: ge data
        """
        if not path_check(PathManager.get_db_path(self._result_dir, DBNameConstant.DB_GE_INFO)):
            return []
        ge_model = ViewModel(self._result_dir, DBNameConstant.DB_GE_INFO, [DBNameConstant.TABLE_GE_TASK])
        ge_model.init()
        ge_sql = 'select stream_id, task_id, timestamp, batch_id from TaskInfo order by timestamp'
        ge_data_list = ge_model.get_sql_data(ge_sql)
        result_list = []
        for ge_data in ge_data_list:
            result_list.append(
                {'timestamp': ge_data[2], 'stream_id': ge_data[0], 'task_id': ge_data[1], 'batch_id': ge_data[3]})
        return result_list
