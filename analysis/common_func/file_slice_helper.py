#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import os

from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.msvp_common import create_csv
from msinterface.msprof_data_storage import MsprofDataStorage


class FileSliceHelper:
    """
    This class is used to slicing a summary file.
    """
    COUNT_LIMIT = 1000000
    COUNT_INIT = 0

    def __init__(self: any, target_dir: str, file_name: str, export_type: str) -> None:
        """
        target_dir: file target_dir
        export_type: summary or timeline
        """
        self.data_list = []
        self.slice_count = self.COUNT_INIT
        self.header = []
        self.params = {
            StrConstant.PARAM_DATA_TYPE: file_name,
            StrConstant.PARAM_EXPORT_TYPE: export_type,
            StrConstant.PARAM_RESULT_DIR: target_dir
        }

    def set_header(self, header: list):
        if not self.header:
            self.header = header

    def check_header_is_empty(self) -> bool:
        return not self.header

    def insert_data(self: any, data_list: list):
        if not data_list:
            return
        self.data_list.extend(data_list)
        self.dump_csv_data()

    def dump_csv_data(self, force: bool = False):
        if force and self.data_list:
            csv_file = make_export_file_name(self.params, self.slice_count,
                                             slice_switch=(self.slice_count != self.COUNT_INIT))
            create_csv(csv_file, self.header, self.data_list[self.slice_count * self.COUNT_LIMIT:], use_dict=True)
            return

        while len(self.data_list) >= ((self.slice_count + 1) * self.COUNT_LIMIT):
            csv_file = make_export_file_name(self.params, self.slice_count, slice_switch=True)
            create_csv(csv_file, self.header,
                       self.data_list[self.slice_count * self.COUNT_LIMIT:
                                      (self.slice_count + 1) * self.COUNT_LIMIT],
                       use_dict=True)
            self.slice_count += 1


def make_export_file_name(params: dict, slice_times: int = 0, slice_switch=False) -> str:
    file_name = MsprofDataStorage.get_export_prefix_file_name(params, slice_times, slice_switch)
    if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.SUMMARY:
        return os.path.join(params.get(StrConstant.PARAM_RESULT_DIR), file_name + StrConstant.FILE_SUFFIX_CSV)
    return os.path.join(params.get(StrConstant.PARAM_RESULT_DIR), file_name + StrConstant.FILE_SUFFIX_JSON)
