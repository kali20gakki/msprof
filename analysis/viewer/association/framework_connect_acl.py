#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.

import os

from common_func.common import CommonConstant, print_info
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.msproftx.msproftx_model import MsprofTxModel
from profiling_bean.prof_enum.data_tag import DataTag


class FrameworkToAcl:
    ACL_OP_NAME = ['aclopCompileAndExecute', 'aclopCompileAndExecuteV2']

    def __init__(self, result_dir: str, json_data: list):
        self._result_dir = result_dir
        self._json_data = json_data
        self._acl_data = []
        self._msproftx_torch_data = []
        self._msproftx_cann_data = []

    def add_connect_line(self) -> list:
        connect_data = []
        self._load_data()
        if self._acl_data and self._msproftx_torch_data and self._msproftx_cann_data:
            start_point_data = self._get_connect_start_point()
            end_point_data = self._get_connect_end_point()
            if not start_point_data or not end_point_data:
                return connect_data
            connect_data.extend(start_point_data)
            connect_data.extend(end_point_data)
        return connect_data

    def _load_data(self):
        for data in self._json_data:
            if data.get('name', '') in FrameworkToAcl.ACL_OP_NAME:
                self._acl_data.append(data)
        host_dir = os.path.join(self._result_dir, 'host')
        model = MsprofTxModel(host_dir, DBNameConstant.DB_MSPROFTX, [DBNameConstant.TABLE_MSPROFTX])
        if model.check_db() and model.check_table():
            self._msproftx_torch_data = model.get_msproftx_data_by_file_tag(DataTag.MSPROFTX_TORCH.value)
            self._msproftx_cann_data = model.get_msproftx_data_by_file_tag(DataTag.MSPROFTX_CANN.value)

    def _get_connect_start_point(self) -> list:
        start_point_list = []
        cann_mark_data_dict = {}
        for cann_mark_data in self._msproftx_cann_data:
            cann_mark_data_dict.setdefault(cann_mark_data.message, []).append(cann_mark_data)
        for torch_data in self._msproftx_torch_data:
            if cann_mark_data_dict.get(torch_data.message, []):
                matched_cann_mark_data = cann_mark_data_dict.get(torch_data.message).pop(0)
                start_point_list.append(
                    {'name': 'connect', 'ph': 's', 'id': matched_cann_mark_data.start_time,
                     'pid': f'0_{torch_data.pid}',
                     'tid': torch_data.tid, 'ts': torch_data.start_time / DBManager.NSTOUS})
        empty_torch_num = sum(len(x) for x in cann_mark_data_dict.values())
        if empty_torch_num:
            print_info(CommonConstant.FILE_NAME,
                       f'The execution of {empty_torch_num}/{len(self._msproftx_cann_data)} operators '
                       f'did not find the corresponding CANN operators delivered by framework.')
        return start_point_list

    def _get_connect_end_point(self) -> list:
        end_point_list = []
        self._msproftx_cann_data.reverse()
        sorted(self._acl_data, key=lambda x: x.get('ts'))
        empty_acl_execute_num = 0
        for cann_mark_data in self._msproftx_cann_data:
            matched_acl_data = None
            while self._acl_data:
                if cann_mark_data.start_time / DBManager.NSTOUS >= self._acl_data[-1].get('ts'):
                    break
                matched_acl_data = self._acl_data.pop()
            if matched_acl_data:
                end_point_list.append(
                    {'name': 'connect', 'ph': 'f', 'id': cann_mark_data.start_time, 'pid': matched_acl_data.get('pid'),
                     'tid': matched_acl_data.get('tid'), 'ts': matched_acl_data.get('ts'), 'bp': 'e'})
            else:
                empty_acl_execute_num += 1
        if empty_acl_execute_num:
            print_info(CommonConstant.FILE_NAME,
                       f'The execution of {empty_acl_execute_num}/{len(self._msproftx_cann_data)} operators '
                       f'did not find the corresponding AscendCL apis.')
        return end_point_list
