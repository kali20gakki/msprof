#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024. All rights reserved.

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.ub_dto import UBDto


class UBModel(ParserModel):
    """
    UB model class
    """

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)

    def flush(self: any, data_list: list) -> None:
        """
        flush UB data to db
        :param data_list: UB data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_UB_BW, data_list)


class UBViewModel(ViewModel):
    """
    UB view model class
    """

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)

    def get_timeline_data(self: any) -> list:
        """
        get UB bandwidth data
        :return: list
        """
        sql = self.get_sql()
        return DBManager.fetch_all_data(self.cur, sql, dto_class=UBDto)

    def get_summary_data(self: any) -> list:
        """
        get UB bandwidth data
        :return: list
        """
        sql = self.get_sql()
        return DBManager.fetch_all_data(self.cur, sql, dto_class=UBDto)

    def get_sql(self):
        return "select device_id, port_id, time_stamp, udma_rx_bind, udma_tx_bind, rx_port_band_width, " \
               "rx_packet_rate, rx_bytes, rx_packets, rx_errors, rx_dropped, tx_port_band_width, tx_packet_rate, " \
               "tx_bytes, tx_packets, tx_errors, tx_dropped from {};".format(DBNameConstant.TABLE_UB_BW)
