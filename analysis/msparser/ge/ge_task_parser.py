#!/usr/bin/python3
# coding=utf-8
"""
function: parser for ge task
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

from common_func.db_name_constant import DBNameConstant
from common_func.empty_class import EmptyClass
from common_func.ms_multi_process import MsMultiProcess
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.ge.ge_task_bean import GeTaskBean
from profiling_bean.prof_enum.data_tag import DataTag


class GeTaskParser(DataParser, MsMultiProcess):
    """
    ge task data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        DataParser.__init__(self, sample_config)
        self._file_list = file_list
        self.data = []

    def parse(self: any) -> None:
        """
        parse ge task data
        """
        fusion_task_file = self._file_list.get(DataTag.GE_TASK, [])
        if fusion_task_file:
            self.data = self.parse_bean_data(fusion_task_file, StructFmt.GE_TASK_SIZE, GeTaskBean, self._get_task_data)

    def _get_task_data(self: any, bean_data: any) -> list:
        if isinstance(bean_data, EmptyClass):
            return []
        op_name = self._get_op_name(bean_data)
        op_type = self._get_op_type(bean_data)
        return [bean_data.model_id, op_name, bean_data.stream_id,
                bean_data.task_id, bean_data.block_dims, bean_data.shape_type, bean_data.task_type,
                op_type, bean_data.index_num,
                bean_data.thread_id, bean_data.timestamp, bean_data.batch_id]

    @staticmethod
    def _get_op_name(bean_data: any) -> any:
        if bean_data.is_op_name_hash_type():
            op_name = bean_data.op_name
        else:
            op_name = bean_data.op_name.partition(b'\x00')[0].decode('utf-8', 'ignore')
        return str(op_name)

    @staticmethod
    def _get_op_type(bean_data: any) -> any:
        if bean_data.is_op_type_hash_type():
            op_type = bean_data.op_type
        else:
            op_type = bean_data.op_type.partition(b'\x00')[0].decode('utf-8', 'ignore')
        return str(op_type)

    @staticmethod
    def read_binary_data(bean_class: any, bean_data: any) -> any:
        """
        read binary data
        """
        return bean_class().fusion_decode(bean_data)

    def save(self: any) -> str:
        """
        save
        :return:
        """
        return str(self.__name__)

    def ms_run(self: any) -> dict:
        """
        parse and save ge task data
        :return:
        """
        if not self._file_list.get(DataTag.GE_TASK, []):
            return {}
        self.parse()
        return {DBNameConstant.TABLE_GE_TASK: self.data}
