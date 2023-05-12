#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

from common_func.db_name_constant import DBNameConstant
from common_func.ms_multi_process import MsMultiProcess
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.ge.ge_tensor_bean import GeTensorBean
from profiling_bean.prof_enum.data_tag import DataTag


class GeTensorParser(DataParser, MsMultiProcess):
    """
    ge tensor data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)
        self._file_list = file_list
        self.data = []

    @staticmethod
    def read_binary_data(bean_class: any, bean_data: any) -> any:
        """
        read binary data
        """
        return bean_class().fusion_decode(bean_data)

    @staticmethod
    def _get_tensor_data(bean_data: any) -> list:
        if not bean_data:
            return []
        return [bean_data.model_id, bean_data.stream_id, bean_data.task_id,
                bean_data.tensor_num, bean_data.input_format, bean_data.input_data_type,
                bean_data.input_shape, bean_data.output_format, bean_data.output_data_type,
                bean_data.output_shape, bean_data.index_num, bean_data.timestamp, bean_data.batch_id]

    @classmethod
    def _generate_new_hash_dict_data(cls: any, hash_dict: dict, key: any, data: list) -> None:
        hash_dict[key] = {
            'tensor_num': data[3],
            'input_format': str(data[4]),
            'input_data_type': str(data[5]),
            'input_shape': str(data[6]),
            'output_format': str(data[7]),
            'output_data_type': str(data[8]),
            'output_shape': str(data[9])
        }

    @classmethod
    def _update_hash_dict_data(cls: any, hash_dict: dict, key: any, data: list) -> None:
        value = hash_dict.get(key, {})
        value['tensor_num'] += data[3]
        # ';'   to avoid filtering the empty data
        value['input_format'] += (";" + str(data[4])) if data[4] else ';'
        value['input_data_type'] += (";" + str(data[5])) if data[5] else ';'
        value['input_shape'] += (";" + str(data[6])) if data[6] else ';'
        value['output_format'] += (";" + str(data[7])) if data[7] else ';'
        value['output_data_type'] += (";" + str(data[8])) if data[8] else ';'
        value['output_shape'] += (";" + str(data[9])) if data[9] else ';'
        hash_dict[key] = value

    def parse(self: any) -> None:
        """
        parse ge tensor data
        """
        fusion_tensor_file = self._file_list.get(DataTag.GE_TENSOR, [])
        if fusion_tensor_file:
            self.data = self.parse_bean_data(fusion_tensor_file,
                                             StructFmt.GE_TENSOR_SIZE,
                                             GeTensorBean,
                                             self._get_tensor_data)
            self._update_tensor_data()

    def save(self: any) -> str:
        """
        save
        :return:
        """
        return str(self.__name__)

    def ms_run(self: any) -> dict:
        """
        parse and save ge tensor data
        :return:
        """
        if not self._file_list.get(DataTag.GE_TENSOR, []):
            return {}
        self.parse()
        return {DBNameConstant.TABLE_GE_TENSOR: self.data}

    def _update_tensor_data(self: any) -> None:
        hash_dict = {}
        for data in self.data:
            if len(data) < 13:
                continue
            # 0 model; 1 stream; 2 task; 10 index; 11 timestamp; 12 batch
            key = "{}_{}_{}_{}_{}_{}".format(
                str(data[0]), str(data[1]), str(data[2]), str(data[10]), str(data[11]), str(data[12]))
            if key not in hash_dict.keys():
                self._generate_new_hash_dict_data(hash_dict, key, data)
            else:
                self._update_hash_dict_data(hash_dict, key, data)
        self._assemble_tensor_data(hash_dict)

    def _assemble_tensor_data(self: any, hash_dict: dict) -> None:
        self.data = []
        for key, value in hash_dict.items():
            model_id = int(key.split("_")[0])
            stream_id = int(key.split("_")[1])
            task_id = int(key.split("_")[2])
            index_id = int(key.split("_")[3])
            timestamp = str(key.split("_")[4])
            batch_id = str(key.split("_")[5])
            self.data.append(
                [model_id, stream_id, task_id, value['tensor_num'], value['input_format'],
                 value['input_data_type'], "\"" + value['input_shape'] + "\"",
                 value['output_format'], value['output_data_type'],
                 "\"" + value['output_shape'] + "\"", index_id, timestamp, batch_id]
            )
