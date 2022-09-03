#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import os

from common_func.msvp_common import error
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.path_manager import PathManager
from msmodel.interface.parser_model import ParserModel


class HwtsLogModel(ParserModel):
    """
    class used to operate hwts log model
    """

    def __init__(self: any, result_dir: str) -> None:
        super(HwtsLogModel, self).__init__(result_dir, DBNameConstant.DB_HWTS,
                                           [DBNameConstant.TABLE_HWTS_TASK, DBNameConstant.TABLE_HWTS_TASK_TIME])

    def flush(self: any, data_list: list, table_name: str) -> None:
        """
        flush to db
        :param data_list: data
        :param table_name: table name
        :return:
        """
        self.insert_data_to_db(table_name, data_list)

    def clear(self: any) -> None:
        """
        clear db file
        :return:
        """
        try:
            if os.path.exists(PathManager.get_db_path(self.result_dir, DBNameConstant.DB_HWTS)):
                os.remove(PathManager.get_db_path(self.result_dir, DBNameConstant.DB_HWTS))
        except (OSError, SystemError) as err:
            error(os.path.basename(__file__), str(err))
