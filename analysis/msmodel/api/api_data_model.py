#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import logging

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.hash_dict_constant import HashDictData
from msmodel.interface.parser_model import ParserModel
from profiling_bean.prof_enum.data_tag import AclApiTag
from profiling_bean.struct_info.api_data_bean import ApiDataBean


class ApiDataModel(ParserModel):
    """
    api model class
    """

    PYTORCH_TX_LEVEL = 'pytorch_tx'
    PTA_LEVEL = 'pta'
    ACL_LEVEL = 'acl'
    MODEL_LEVEL = 'model'
    NODE_LEVEL = 'node'
    HCCL_LEVEL = 'hccl'
    RUNTIME_LEVEL = 'runtime'

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_API_EVENT, [DBNameConstant.TABLE_API_DATA])

    @staticmethod
    def update_hash_value(data: ApiDataBean, hash_dict_data: HashDictData) -> tuple:
        type_dict = hash_dict_data.get_type_hash_dict()
        ge_dict = hash_dict_data.get_ge_hash_dict()

        item_id = ge_dict.get(data.item_id, data.item_id)
        if data.level not in type_dict:
            struct_type = data.struct_type
            data_id = data.struct_type
        else:
            if data.level == ApiDataModel.ACL_LEVEL:
                struct_type = AclApiTag(data.acl_type).name
                data_id = type_dict[data.level].get(data.struct_type, data.struct_type)
            elif data.level == ApiDataModel.HCCL_LEVEL:
                struct_type = type_dict[data.level].get(data.struct_type, data.struct_type)
                data_id = item_id
            else:
                struct_type = type_dict[data.level].get(data.struct_type, data.struct_type)
                data_id = type_dict[data.level].get(data.struct_type, data.struct_type)

        return struct_type, data_id, item_id

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_API_DATA) -> None:
        """
        insert data to table
        :param data_list: api data
        :param table_name: table name
        :return:
        """
        try:
            data_list = self.reformat_data(data_list)
        except (IndexError, ValueError) as _err:
            logging.error(str(_err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.insert_data_to_db(table_name, data_list)

    def reformat_data(self: any, data_list: list) -> list:
        hash_dict_data = HashDictData(self.result_dir)
        res = []
        for data in data_list:
            struct_type, data_id, item_id = self.update_hash_value(data, hash_dict_data)
            res.append([struct_type, data_id, data.level, data.thread_id, item_id, data.start, data.end])
        return res
