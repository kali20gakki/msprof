#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.ccu.ccu_mission_dto import OriginMissionDto


class CCUMissionModel(ParserModel):
    """
    ccu mission model class
    """

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)

    def flush(self: any, data_list: list, table_name: str) -> None:
        """
        flush ccu mission data to db
        :param data_list: ccu mission origin data list
        :return: None
        """
        self.insert_data_to_db(table_name, data_list)


class CCUViewerMissionModel(ViewModel):
    """
    ccu viewer mission model class
    """

    def __init__(self: any, result_dir: str, *args, **kwargs) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CCU, [DBNameConstant.TABLE_CCU_MISSION])

    def get_timeline_data(self: any) -> list:
        """
        get ccu mission data
        :return: list
        """
        sql = ("SELECT stream_id, task_id, lp_instr_id, setckebit_instr_id, rel_id, " \
               "CASE WHEN setckebit_start_time <> 0 THEN setckebit_start_time ELSE lp_start_time END AS start_time, " \
               "CASE WHEN setckebit_start_time <> 0 THEN rel_end_time ELSE lp_end_time END AS end_time, " \
               "CASE WHEN setckebit_start_time <> 0 THEN 'Wait' ELSE 'LoopGroup' END AS time_type FROM {} " \
               "WHERE setckebit_start_time <> 0 OR lp_start_time <> 0;"
               .format(DBNameConstant.TABLE_CCU_MISSION))
        return DBManager.fetch_all_data(self.cur, sql, dto_class=OriginMissionDto)

    def get_summary_data(self: any) -> list:
        """
        get ccu mission data
        :return: list
        """
        sql = "select stream_id, task_id, lp_instr_id, lp_start_time, lp_end_time, " \
              "setckebit_instr_id, setckebit_start_time, rel_id, rel_end_time from {};" \
            .format(DBNameConstant.TABLE_CCU_MISSION)
        return DBManager.fetch_all_data(self.cur, sql, dto_class=OriginMissionDto)
