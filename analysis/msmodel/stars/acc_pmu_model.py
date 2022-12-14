#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

from msconfig.config_manager import ConfigManager
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel


class AccPmuModel(ParserModel):
    """
    task_based acc_pmu data model
    """

    READ = 1
    WRITE = 0
    TABLES_PATH = ConfigManager.TABLES

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.cur = None
        self.conn = None

    def insert_pmu_origin_data(self: any, datas: list) -> None:
        """
        insert acc_pmu data to db
        :return: None
        """
        result = [(data.stars_common.task_id, data.stars_common.stream_id, data.acc_id, data.block_id,
                   data.bandwidth[self.READ], data.bandwidth[self.WRITE], data.ost[self.READ], data.ost[self.WRITE],
                   data.stars_common.timestamp) for data in datas]

        self.insert_data_to_db(DBNameConstant.TABLE_ACC_PMU_ORIGIN_DATA, result)

    def flush(self: any, data: list) -> None:
        """
        insert data to database
        """
        self.insert_pmu_origin_data(data)

    def get_timeline_data(self: any) -> list:
        """
        get soc timeline data
        :return: list
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_ACC_PMU_DATA):
            return []
        sql = "select task_id, stream_id, acc_id, block_id, " \
              " read_bandwidth, write_bandwidth ,read_ost, write_ost, " \
              "timestamp, start_time, dur_time from {}".format(DBNameConstant.TABLE_ACC_PMU_DATA)
        data_list = DBManager.fetch_all_data(self.cur, sql)
        return data_list

    def get_summary_data(self: any) -> list:
        """
        get soc timeline data
        :return: list
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_ACC_PMU_DATA):
            return []
        sql = "select task_id, stream_id, acc_id, block_id," \
              " read_bandwidth, write_bandwidth ,read_ost, write_ost, " \
              "time_stamp, start_time, dur_time from {}".format(DBNameConstant.TABLE_ACC_PMU_DATA)
        return DBManager.fetch_all_data(self.cur, sql)
