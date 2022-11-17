#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import logging
import os

from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager, FileOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from msmodel.l2_cache.l2_cache_parser_model import L2CacheParserModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.l2_cache_sample import L2CacheSampleDataBean


class L2CacheSampleParser(IParser, MsMultiProcess):
    """
    l2 cache data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super(L2CacheSampleParser, self).__init__(sample_config)
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._file_list = file_list
        self._model = L2CacheParserModel(self._project_path, [DBNameConstant.TABLE_L2CACHE_SAMPLE])
        self._l2_cache_data = []

    def save(self: any) -> None:
        """
        save the result of data parsing
        :return: NA
        """
        if not self._l2_cache_data:
            return
        with self._model as _model:
            self._model.flush(self._l2_cache_data)

    def parse(self: any) -> None:
        """
        parse the data under the file path
        :return: NA
        """
        l2_cache_files = self._file_list.get(DataTag.L2CACHE, [])
        for _file in l2_cache_files:
            _file_path = PathManager.get_data_file_path(self._project_path, _file)

            _file_size = os.path.getsize(_file_path)
            with FileOpen(_file_path, 'rb') as _l2_cache_file:
                _all_l2_cache_data = _l2_cache_file.file_reader.read(_file_size)
                for _index in range(_file_size // StructFmt.L2_CACHE_DATA_SIZE):
                    l2_cache_data_bean = L2CacheSampleDataBean()
                    l2_cache_data_bean.decode(
                        _all_l2_cache_data[
                        _index * StructFmt.L2_CACHE_DATA_SIZE:(_index + 1) * StructFmt.L2_CACHE_DATA_SIZE])
                    self._l2_cache_data.append(l2_cache_data_bean.events_list)
            FileManager.add_complete_file(self._project_path, _file_path)

    def ms_run(self: any) -> None:
        """
        main entry
        """
        if self._file_list.get(DataTag.L2CACHE):
            logging.info("start parsing l2 cache sample data, files: %s",
                         str(self._file_list.get(DataTag.L2CACHE)))
            self.parse()
            self.save()
