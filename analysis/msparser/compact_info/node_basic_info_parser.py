#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import logging
import sqlite3

from common_func.constant import Constant
from common_func.hash_dict_constant import HashDictData
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from msmodel.compact_info.node_basic_info_model import NodeBasicInfoModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from msparser.compact_info.node_basic_info_bean import NodeBasicInfoBean
from profiling_bean.prof_enum.data_tag import DataTag


class NodeBasicInfoParser(DataParser, MsMultiProcess):
    """
    Node Basic Info Data Parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)
        self._file_list = file_list
        self._node_basic_info_data = []
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

    @staticmethod
    def read_binary_data(bean_class: any, bean_data: any) -> any:
        """
        read binary data
        """
        return bean_class().fusion_decode(bean_data)

    @staticmethod
    def _get_node_basic_data(bean_data: any) -> list:
        if not bean_data:
            return []
        return [bean_data.level, bean_data.add_info_type, bean_data.thread_id, bean_data.timestamp,
                bean_data.node_id, bean_data.task_type, bean_data.op_type, bean_data.block_dim,
                bean_data.mix_block_dim, bean_data.op_flag]

    def parse(self: any) -> None:
        """
        parse node basic data
        """
        basic_info_files = self._file_list.get(DataTag.NODE_BASIC_INFO, [])
        basic_info_files = self.group_aging_file(basic_info_files)
        for file_list in basic_info_files.values():
            self._node_basic_info_data.extend(self.parse_bean_data(file_list, StructFmt.NODE_BASIC_INFO_SIZE,
                                                                   NodeBasicInfoBean, self._get_node_basic_data))

    def save(self: any) -> None:
        """
        save
        :return:
        """
        if not self._node_basic_info_data:
            return
        model = NodeBasicInfoModel(self._project_path)
        with model as _model:
            _model.flush(self._transform_fusion_info_data(self._node_basic_info_data))

    def ms_run(self: any) -> None:
        """
        parse and save node basic data
        :return:
        """
        if not self._file_list.get(DataTag.NODE_BASIC_INFO, []):
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

    def _transform_fusion_info_data(self: any, data_list: list) -> list:
        hash_dict_data = HashDictData(self._project_path)
        type_hash_dict = hash_dict_data.get_type_hash_dict()
        ge_hash_dict = hash_dict_data.get_ge_hash_dict()
        for data in data_list:
            # 1 type hash, 4 node hash, 6 op type
            data[1] = type_hash_dict.get('node', {}).get(data[1], 'unmatched')
            data[4] = ge_hash_dict.get(data[4], 'unmatched')
            data[6] = ge_hash_dict.get(data[6], 'unmatched')
        return data_list
