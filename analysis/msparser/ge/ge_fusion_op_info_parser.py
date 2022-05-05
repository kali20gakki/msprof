#!/usr/bin/python3
# coding=utf-8
"""
function: parser for ge fusion op info
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
from common_func.db_name_constant import DBNameConstant
from common_func.ms_multi_process import MsMultiProcess
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.ge.ge_fusion_op_info_bean import GeFusionOpInfoBean
from profiling_bean.prof_enum.data_tag import DataTag


class GeFusionOpParser(DataParser, MsMultiProcess):
    """
    ge fusion op info data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        DataParser.__init__(self, sample_config)
        self._file_list = file_list
        self.data = []
        self.table_list = []

    def parse(self: any) -> None:
        """
        parse fusion op data
        """
        fusion_op_file = self._file_list.get(DataTag.GE_FUSION_OP_INFO, [])
        if fusion_op_file:
            self.data = self.parse_bean_data(fusion_op_file,
                                             StructFmt.GE_FUSION_OP_SIZE,
                                             GeFusionOpInfoBean,
                                             self._get_fusion_op_info_data)

    @staticmethod
    def read_binary_data(bean_class: any, bean_data: any) -> any:
        """
        read binary data
        """
        return bean_class().fusion_decode(bean_data)

    @staticmethod
    def _get_fusion_op_info_data(bean_data: any) -> list:
        if not bean_data:
            return []
        fusion_name = bean_data.fusion_name
        if not bean_data.is_hash_type():
            fusion_name = bean_data.fusion_name.partition(b'\x00')[0].decode('utf-8', 'ignore')
        fusion_name = str(fusion_name)
        return [bean_data.model_id, fusion_name, bean_data.fusion_op_num,
                bean_data.fusion_op, bean_data.input_mem_size, bean_data.output_mem_size,
                bean_data.weight_mem_size, bean_data.workspace_mem_size, bean_data.total_mem_size]

    def save(self: any) -> str:
        """
        save
        :return:
        """
        return str(self.__name__)

    def ms_run(self: any) -> dict:
        """
        parse ge fusion op info  data
        :return:
        """
        if not self._file_list.get(DataTag.GE_FUSION_OP_INFO, []):
            return {}

        self.parse()
        return {DBNameConstant.TABLE_GE_FUSION_OP_INFO: self.data}
