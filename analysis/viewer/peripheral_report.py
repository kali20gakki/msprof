# coding:utf-8
"""
This script is amid to provide peripheral reports.
Copyright Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
"""
import sqlite3

from common_func.db_manager import DBManager
from common_func.get_table_data import GetTableData
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import is_number
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant


def get_peripheral_dvpp_data(db_path: str, table_name: str, device_id: str, configs: dict) -> tuple:
    """
    get peripheral dvpp data
    """
    conn, curs = DBManager.check_connect_db_path(db_path)
    if not (conn and curs and DBManager.judge_table_exist(curs, table_name)):
        return MsvpConstant.MSVP_EMPTY_DATA
    data = get_dvpp_data(curs, device_id)
    try:
        count = GetTableData.get_data_count_for_device(curs, table_name, device_id)
        return configs.get(StrConstant.CONFIG_HEADERS), data, count
    except sqlite3.Error:
        return MsvpConstant.MSVP_EMPTY_DATA
    finally:
        DBManager.destroy_db_connect(conn, curs)


def get_dvpp_data(curs: any, devid: str) -> list:
    """
    get dvpp data
    """
    report_data = DBManager.fetch_all_data(curs, "select * from DvppReportData where device_id = ?;", (devid,))
    new_report_data = []
    for rd in report_data:
        item = [rd[0], StrConstant.DVPP_ENGINE_TYPE.get(rd[2])]
        item.extend(rd[3:])
        new_report_data.append(item)
    res = []
    for _, items in enumerate(new_report_data):
        result_list_data = list(items)
        data = result_list_data[-1].split("%")
        if data and is_number(str(data[0])):
            result_list_data[-1] = str(StrConstant.ACCURACY % round(float(data[0]), NumberConstant.DECIMAL_ACCURACY))
        res.append(tuple(result_list_data))
    return res


def get_peripheral_nic_data(db_path: str, table_name: str, device_id: str, configs: dict) -> tuple:
    """
    get peripheral nic data
    """
    conn, curs = DBManager.check_connect_db_path(db_path)
    if not (conn and curs and DBManager.judge_table_exist(curs, table_name)):
        return MsvpConstant.MSVP_EMPTY_DATA
    data = GetTableData.get_table_data_for_device(curs, table_name, device_id,
                                                  configs.get(StrConstant.CONFIG_COLUMNS))
    try:
        count = GetTableData.get_data_count_for_device(curs, table_name, device_id)
        return configs.get(StrConstant.CONFIG_HEADERS), data, count
    except sqlite3.Error:
        return MsvpConstant.MSVP_EMPTY_DATA
    finally:
        DBManager.destroy_db_connect(conn, curs)
