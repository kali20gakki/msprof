#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import os

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.msvp_common import MsvpCommonConst
from msmodel.interface.parser_model import ParserModel


class MsprofTxModel(ParserModel):
    """
    db operator for msproftx parser
    """
    TABLES_PATH = os.path.join(MsvpCommonConst.CONFIG_PATH, 'Tables.ini')

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
                       "message, event_type from {}".format(DBNameConstant.TABLE_MSPROFTX)
        return DBManager.fetch_all_data(self.cur, all_data_sql)
