#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.biu_perf_insrt_dto import BiuPerfInstrDto


class BiuPerfChip6Model(ParserModel):
    """
    db operator for biu perf chip6 parser
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_BIU_PERF, table_list)

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_BIU_DATA) -> None:
        """
        flush biu perf data to db
        :param data_list: biu instr data list
        :return: None
        """
        self.insert_data_to_db(table_name, data_list)


class BiuPerfChip6ViewerModel(ViewModel):
    """
    db operator for biu perf chip6 parser
    """

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)

    def get_timeline_data(self: any) -> list:
        """
        get biu perf instruction status data
        :return: list
        """
        sql = "select group_id, core_type, block_id, instruction, timestamp, duration, checkpoint_info from {} " \
            .format(DBNameConstant.TABLE_BIU_INSTR_STATUS)
        return DBManager.fetch_all_data(self.cur, sql, dto_class=BiuPerfInstrDto)
