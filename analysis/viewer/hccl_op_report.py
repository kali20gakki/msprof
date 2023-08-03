#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2023. All rights reserved.

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_constant import MsvpConstant


class ReportHcclStatisticData:
    """
    class to report hccl op data
    """
    @staticmethod
    def check_param(conn: any, curs: any) -> bool:
        """
        check exist of db table
        """
        if not (conn and curs) or not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_HCCL_OP_REPORT):
            return False
        return True

    @staticmethod
    def _get_hccl_op_report_sql() -> str:
        sql = "select op_type,  occurrences, total_time/{NS_TO_US}, " \
              "min/{NS_TO_US}, avg/{NS_TO_US}, max/{NS_TO_US}, ratio from {0} " \
              "order by total_time desc " \
              .format(DBNameConstant.TABLE_HCCL_OP_REPORT, NS_TO_US=NumberConstant.NS_TO_US)
        return sql

    @classmethod
    def report_hccl_op(cls: any, db_path: str, headers: list) -> tuple:
        """
        report hccl op
        :param db_path: DB path
        :param headers: table headers
        :return: headers, data, data length
        """
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not cls.check_param(conn, curs):
            return MsvpConstant.MSVP_EMPTY_DATA
        sql = cls._get_hccl_op_report_sql()
        data = DBManager.fetch_all_data(curs, sql)
        DBManager.destroy_db_connect(conn, curs)
        return headers, data, len(data)