#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

from abc import ABC
from common_func.db_name_constant import DBNameConstant
from msconfig.config_manager import ConfigManager
from msmodel.interface.parser_model import ParserModel


class NetDevStatsModel(ParserModel):
    """
    netdev stats model class
    """
    TABLES_PATH = ConfigManager.TABLES_TRAINING

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_NETDEV_STATS, [DBNameConstant.TABLE_NETDEV_STATS_ORIGIN])

    def flush(self: any, data_list: list) -> None:
        """
        flush netdev stats data to db
        :param data_list: netdev stats data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_NETDEV_STATS_ORIGIN, data_list)
