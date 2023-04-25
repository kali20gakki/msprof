#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import logging
import sqlite3

from common_func.constant import Constant
from common_func.hash_dict_constant import HashDictData
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from msmodel.add_info.graph_add_info_model import GraphAddInfoModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from msparser.add_info.graph_add_info_bean import GraphAddInfoBean
from profiling_bean.prof_enum.data_tag import DataTag


class GraphAddInfoParser(DataParser, MsMultiProcess):
    """
    Graph Add Data Parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._graph_info_data = []
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

    @staticmethod
    def read_binary_data(bean_class: any, bean_data: any) -> any:
        """
        read binary data
        """
        return bean_class().fusion_decode(bean_data)

    @staticmethod
    def _get_graph_info_data(bean_data: any) -> list:
        if not bean_data:
            return []
        return [bean_data.level, bean_data.add_info_type, bean_data.thread_id,
                bean_data.timestamp, bean_data.graph_id, bean_data.model_name]

    def parse(self: any) -> None:
        """
        parse ge graph data
        """
        graph_info_files = self._file_list.get(DataTag.GRAPH_ADD_INFO, [])
        graph_info_files = self.group_aging_file(graph_info_files)
        for file_list in graph_info_files.values():
            self._graph_info_data.extend(self.parse_bean_data(file_list, StructFmt.GRAPH_ADD_INFO_SIZE,
                                                              GraphAddInfoBean, self._get_graph_info_data))

    def save(self: any) -> None:
        """
        save data
        """
        if not self._graph_info_data:
            return
        model = GraphAddInfoModel(self._project_path)
        with model as _model:
            _model.flush(self._transform_graph_info_data(self._graph_info_data))

    def ms_run(self: any) -> None:
        """
        parse and save ge graph data
        :return:
        """
        if not self._file_list.get(DataTag.GRAPH_ADD_INFO, []):
            return
        try:
            self.parse()
        except (OSError, IOError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        try:
            self.save()
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _transform_graph_info_data(self: any, data_list: list) -> list:
        hash_dict_data = HashDictData(self._project_path)
        type_hash_dict = hash_dict_data.get_type_hash_dict()
        for data in data_list:
            # 1 type hash
            data[1] = type_hash_dict.get('model', {}).get(data[1], 'unmatched')
        return data_list
