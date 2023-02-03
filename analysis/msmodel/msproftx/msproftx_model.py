#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
from common_func.ms_constant.number_constant import NumberConstant
from msconfig.config_manager import ConfigManager
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from profiling_bean.db_dto.msproftx_dto import MsprofTxDto
from profiling_bean.prof_enum.data_tag import DataTag


class MsprofTxModel(ParserModel):
    """
    db operator for msproftx parser
    """
    TABLES_PATH = ConfigManager.TABLES

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
        all_data_sql = "select category, pid, tid, start_time, (end_time-start_time)" \
                       " as dur_time, payload_type, payload_value, message_type, " \
                       "message, event_type, call_trace from {} where file_tag<>?".format(DBNameConstant.TABLE_MSPROFTX)
        return DBManager.fetch_all_data(self.cur, all_data_sql, (DataTag.MSPROFTX_CANN.value,), dto_class=MsprofTxDto)

    def get_msproftx_data_by_file_tag(self: any, file_tag: int):
        sql = "select pid, tid, start_time, message from {} where file_tag=? order by start_time".format(
            DBNameConstant.TABLE_MSPROFTX)
        return DBManager.fetch_all_data(self.cur, sql, (file_tag,), dto_class=MsprofTxDto)

    def get_summary_data(self: any) -> list:
        all_data_sql = "select pid, tid, category, event_type, payload_type, payload_value, start_time, " \
              "end_time, message_type, message, call_trace from {} where file_tag<>?".format(
            DBNameConstant.TABLE_MSPROFTX)
        return DBManager.fetch_all_data(self.cur, all_data_sql, (DataTag.MSPROFTX_CANN.value,))
