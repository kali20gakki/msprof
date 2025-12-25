#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.ccu.ccu_channel_dto import OriginChannelDto


class CcuChannelModel(ParserModel):
    """
    ccu mission model class
    """

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)

    def flush(self: any, data_list: list, table_name: str) -> None:
        """
        flush ccu mission data to db
        :param data_list: ccu mission data list
        :return: None
        """
        self.insert_data_to_db(table_name, data_list)


class CCUViewerChannelModel(ViewModel):
    """
    ccu viewer channel model class
    """

    def __init__(self: any, result_dir: str, *args, **kwargs) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CCU, [DBNameConstant.TABLE_CCU_CHANNEL])

    def get_summary_data(self: any) -> list:
        """
        get ccu mission data
        :return: list
        """
        sql = "select channel_id, timestamp, max_bw, min_bw, avg_bw from {} where timestamp <> 0;" \
            .format(DBNameConstant.TABLE_CCU_CHANNEL)
        return DBManager.fetch_all_data(self.cur, sql, dto_class=OriginChannelDto)

    def get_timeline_data(self: any) -> list:
        """
        get ccu mission data
        :return: list
        """
        sql = "select channel_id, timestamp, max_bw, min_bw, avg_bw from {} where timestamp <> 0;" \
            .format(DBNameConstant.TABLE_CCU_CHANNEL)
        return DBManager.fetch_all_data(self.cur, sql, dto_class=OriginChannelDto)