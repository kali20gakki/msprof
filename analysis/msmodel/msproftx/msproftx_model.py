#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msconfig.config_manager import ConfigManager
from msmodel.interface.parser_model import ParserModel
from profiling_bean.db_dto.msproftx_dto import MsprofTxDto, MsprofTxExDto
from profiling_bean.prof_enum.data_tag import DataTag


class MsprofTxModel(ParserModel):
    """
    db operator for msproftx parser
    """
    TABLES_PATH = ConfigManager.TABLES

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)

    def flush(self: any, data_list: list) -> None:
        """
        flush msproftx data to db
        :param data_list:msproftx data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_MSPROFTX, data_list)

    def get_timeline_data(self: any) -> list:
        """
        get timeline data
        """
        all_data_sql = f"select category, pid, tid, start_time, (end_time-start_time) as dur_time, payload_type, " \
                       f"payload_value, message_type, message, event_type " \
                       f"from {DBNameConstant.TABLE_MSPROFTX} where file_tag={DataTag.MSPROFTX.value}"
        return DBManager.fetch_all_data(self.cur, all_data_sql, dto_class=MsprofTxDto)

    def get_summary_data(self: any) -> list:
        all_data_sql = f"select pid, tid, category, event_type, payload_type, payload_value, start_time, " \
                       f"end_time, message_type, message from {DBNameConstant.TABLE_MSPROFTX} " \
                       f"where file_tag={DataTag.MSPROFTX.value}"
        return DBManager.fetch_all_data(self.cur, all_data_sql)


class MsprofTxExModel(ParserModel):
    """
    db operator for msproftx ex parser
    """
    def __init__(self: any, result_dir: str):
        super().__init__(result_dir, DBNameConstant.DB_MSPROFTX, [DBNameConstant.TABLE_MSPROFTX_EX])

    def flush(self: any, data_list: list) -> None:
        """
        flush msproftx ex data to db
        :param data_list: msproftx ex data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_MSPROFTX_EX, data_list)

    def get_timeline_data(self) -> list:
        """
        get timeline data
        """
        all_data_sql = f"select pid, tid, event_type, start_time, (end_time-start_time) as dur_time, " \
                       f"mark_id, message from {DBNameConstant.TABLE_MSPROFTX_EX}"
        return DBManager.fetch_all_data(self.cur, all_data_sql, dto_class=MsprofTxExDto)

    def get_summary_data(self) -> list:
        """
        get timeline data
        """
        all_data_sql = f"select pid, tid, event_type, start_time, end_time, " \
                       f"message from {DBNameConstant.TABLE_MSPROFTX_EX}"
        return DBManager.fetch_all_data(self.cur, all_data_sql)
