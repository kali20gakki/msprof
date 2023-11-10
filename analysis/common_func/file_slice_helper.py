#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import json
import logging
import os

from common_func.constant import Constant
from common_func.file_manager import FdOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.msvp_common import create_csv
from msinterface.msprof_data_storage import MsprofDataStorage


class FileSliceHelper:
    """
    This class is used to slicing file.
    """
    # csv count limit, less than 1000000
    CSV_LIMIT = 1000000
    # json file size less than 200M, 300 means a record is 300 Bytes
    JSON_LIMIT = 200 * 1024 * 1024 // 300
    COUNT_INIT = 0

    def __init__(self: any, target_dir: str, file_name: str, export_type: str) -> None:
        """
        target_dir: file target_dir
        export_type: summary or timeline
        """
        self.data_list = []
        # self.file_name_slice used to get the right file name
        self.file_name_slice = self.COUNT_INIT
        self.header = []
        self.params = {
            StrConstant.PARAM_DATA_TYPE: file_name,
            StrConstant.PARAM_EXPORT_TYPE: export_type,
            StrConstant.PARAM_RESULT_DIR: target_dir
        }
        self.connection_id_set = set()

    def set_header(self, header: list):
        if not self.header:
            self.header = header

    def check_header_is_empty(self) -> bool:
        return not self.header

    def insert_data(self: any, data_list: list):
        if not data_list:
            return
        if self.params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.SUMMARY:
            self.data_list.extend(data_list)
            self.dump_csv_data()
        elif self.params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            self._pretreat_json_data(data_list)

    def dump_json_data(self, slice_index: int, is_need_slice: bool = False):
        if self.data_list:
            json_file = make_export_file_name(self.params, slice_index, slice_switch=is_need_slice)
            self._create_json(json_file)

    def dump_csv_data(self, force: bool = False):
        if force and self.data_list:
            csv_file = make_export_file_name(self.params, self.file_name_slice,
                                             slice_switch=(self.file_name_slice != self.COUNT_INIT))
            create_csv(csv_file, self.header, self.data_list, use_dict=True)
            return

        # to count slice in data_list
        slice_count = 0
        while len(self.data_list) >= ((slice_count + 1) * self.CSV_LIMIT):
            csv_file = make_export_file_name(self.params, self.file_name_slice, slice_switch=True)
            create_csv(csv_file, self.header,
                       self.data_list[slice_count * self.CSV_LIMIT:
                                      (slice_count + 1) * self.CSV_LIMIT],
                       use_dict=True)
            slice_count += 1
            self.file_name_slice += 1
        # clear used data to avoid oom
        self.data_list = self.data_list[slice_count * self.CSV_LIMIT:]

    def _pretreat_json_data(self, json_data: list):
        """
        data deduplication in cann level and get timeline header
        filter cann data by connection id
        """
        for data in json_data:
            if data.get('ph', '') == "M":
                self.header.append(data)
                continue
            if "@" in data.get("name"):
                connection_id = data.get("args", {"connection_id": None}).get("connection_id")
                if connection_id and connection_id in self.connection_id_set:
                    continue
                self.connection_id_set.add(connection_id)
            self.data_list.append(data)

    def _create_json(self, filename: str):
        try:
            with FdOpen(filename) as trace_file:
                trace_file.write(json.dumps(self.header + self.data_list))
        except (OSError, SystemError, ValueError, TypeError,
                RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)


def make_export_file_name(params: dict, slice_times: int = 0, slice_switch=False) -> str:
    file_name = MsprofDataStorage.get_export_prefix_file_name(params, slice_times, slice_switch)
    if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.SUMMARY:
        return os.path.join(params.get(StrConstant.PARAM_RESULT_DIR), file_name + StrConstant.FILE_SUFFIX_CSV)
    return os.path.join(params.get(StrConstant.PARAM_RESULT_DIR), file_name + StrConstant.FILE_SUFFIX_JSON)
