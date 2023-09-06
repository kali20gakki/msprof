#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import csv
import logging
import os

from common_func.constant import Constant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant
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

    def insert_data(self: any, data_list: list):
        self.data_list.extend(data_list)
        self.dump_csv_data()

    def dump_csv_data(self, force: bool = False):
        if force and self.data_list:
            csv_file = make_export_file_name(self.params, self.slice_count,
                                             self.slice_count != self.COUNT_INIT)
            self._create_csv(self.data_list, csv_file)
            return

        while len(self.data_list) >= self.COUNT_LIMIT:
            slice_data = self.data_list[:self.COUNT_LIMIT]
            self.data_list = self.data_list[self.COUNT_LIMIT:]
            csv_file = make_export_file_name(self.params, self.slice_count, True)
            self._create_csv(slice_data, csv_file)
            self.slice_count += 1

    def _create_csv(self, data_list: list, csv_file: str):
        """
        create or open csv table as append write
        :param csv_file: csv file path
        :return: result for creating csv file
        """
        try:
            with os.fdopen(os.open(csv_file, Constant.WRITE_FLAGS,
                                   Constant.WRITE_MODES), 'w', newline='') as _csv_file:
                os.chmod(csv_file, NumberConstant.FILE_AUTHORITY)
                if not self.header:
                    logging.warning("Can not get " + csv_file + "'s header!")
                    return
                writer = csv.DictWriter(_csv_file, fieldnames=self.header)
                writer.writeheader()
                while NumberConstant.DATA_NUM < len(data_list):
                    writer.writerows(data_list[:NumberConstant.DATA_NUM])
                    data_list = data_list[NumberConstant.DATA_NUM:]
                writer.writerows(data_list)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.info(str(err))
        finally:
            pass


def make_export_file_name(params: dict, slice_times: int = 0, slice_switch=False) -> str:
    file_name = MsprofDataStorage.get_export_prefix_file_name(params, slice_times, slice_switch)
    if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.SUMMARY:
        return os.path.join(params.get(StrConstant.PARAM_RESULT_DIR), file_name + StrConstant.FILE_SUFFIX_CSV)
    return os.path.join(params.get(StrConstant.PARAM_RESULT_DIR), file_name + StrConstant.FILE_SUFFIX_JSON)