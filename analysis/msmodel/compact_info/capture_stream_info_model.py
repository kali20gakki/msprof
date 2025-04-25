#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel


class CaptureStreamInfoModel(ParserModel):
    """
    db capture stream info
    """

    def __init__(self: any, result_dir: str):
        super().__init__(result_dir, DBNameConstant.DB_STREAM_INFO, [DBNameConstant.TABLE_CAPTURE_STREAM_INFO])

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_CAPTURE_STREAM_INFO) -> None:
        """
        insert data into database
        """
        if not self.table_list:
            return
        self.insert_data_to_db(table_name, data_list)
