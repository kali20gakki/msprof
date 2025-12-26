#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.runtime_op_info_dto import RuntimeOpInfoDto


class RuntimeOpInfoModel(ParserModel):
    """
    Model for runtime op info parser
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_RTS_TRACK, [DBNameConstant.TABLE_RUNTIME_OP_INFO])

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_RUNTIME_OP_INFO) -> None:
        self.insert_data_to_db(table_name, data_list)


class RuntimeOpInfoViewModel(ViewModel):
    def __init__(self: any, path: str) -> None:
        super().__init__(path, DBNameConstant.DB_RTS_TRACK, [])

    def get_runtime_op_info_data(self):
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_RUNTIME_OP_INFO):
            return {}
        sql = ("SELECT TRUE AS is_valid, device_id, model_id, stream_id, task_id, "
               "op_name, task_type, op_type, block_dim, mix_block_dim, op_flag, is_dynamic, tensor_num, "
               "input_formats, input_data_types, input_shapes, output_formats, output_data_types, output_shapes "
               "FROM {}").format(DBNameConstant.TABLE_RUNTIME_OP_INFO)
        op_info_data = DBManager.fetch_all_data(self.cur, sql, dto_class=RuntimeOpInfoDto)
        return {(info.device_id, info.stream_id, info.task_id): info for info in op_info_data}

