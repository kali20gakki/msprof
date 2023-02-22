#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import logging
import os

from common_func.constant import Constant
from common_func.file_manager import FileManager
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from framework.offset_calculator import OffsetCalculator
from msparser.acl.acl_data_bean import AclDataBean
from msparser.acl.acl_sql_parser import AclSqlParser
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag


class AclParser(IParser, MsMultiProcess):
    """
    acl data parser
    """
    ACL_DATA_SIZE = 64

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._acl_data = []
        self._device_id = self._sample_config.get("device_id", "0")

    @staticmethod
    def group_file_list(file_list: list) -> dict:
        """
        Grouping Files
        :return: dict
        """
        result_dict = {'acl_model': [], 'acl_op': [], 'acl_rts': [], 'acl_others': []}
        for file in file_list:
            if file.split('.')[1] in result_dict.keys():
                result_dict.setdefault(file.split('.')[1], []).append(file)
        return result_dict

    def parse(self: any) -> None:
        """
        parse function
        """
        acl_files = self._file_list.get(DataTag.ACL, [])
        acl_files = self.group_file_list(acl_files)
        for file_list in acl_files.values():
            if not file_list:
                continue
            file_list.sort(key=lambda x: int(x.split("_")[-1]))
            for _file in file_list:
                _file_path = PathManager.get_data_file_path(self._project_path, _file)

                _file_size = os.path.getsize(_file_path)
                if not _file_size:
                    return
                logging.info(
                    "start parsing acl data file: %s", _file)
                self._insert_acl_data(_file_path, _file_size, file_list)
                FileManager.add_complete_file(self._project_path, _file)

    def save(self: any) -> None:
        """
        save data to db
        :return:
        """
        AclSqlParser.create_acl_data_table(self._project_path)
        AclSqlParser.insert_acl_data_to_db(self._project_path, self._acl_data)
        logging.info("Create acl_module DB finished!")

    def ms_run(self: any) -> None:
        """
        entrance for acl parser
        :return:
        """
        if not self._file_list.get(DataTag.ACL, []):
            return

        try:
            self.parse()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.save()

    def _insert_acl_data(self: any, file_path: str, file_size: int, file_list: list) -> None:
        offset_calculator = OffsetCalculator(file_list, StructFmt.ACL_FMT_SIZE,
                                             self._project_path)
        struct_size = StructFmt.ACL_FMT_SIZE
        with open(file_path, 'rb') as _acl_file:
            _all_acl_data = offset_calculator.pre_process(_acl_file, file_size)
            for _index in range(file_size // struct_size):
                acl_data_bean = AclDataBean().acl_decode(
                    _all_acl_data[_index * struct_size:(_index + 1) * struct_size])

                if acl_data_bean:
                    self._acl_data.append([
                        acl_data_bean.api_name,
                        acl_data_bean.api_type,
                        acl_data_bean.api_start,
                        acl_data_bean.api_end,
                        acl_data_bean.process_id,
                        acl_data_bean.thread_id,
                        self._device_id
                    ])