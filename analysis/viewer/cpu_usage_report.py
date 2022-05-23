#!/usr/bin/python3
# coding:utf-8
"""
get and report cpu usage data
Copyright Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
"""

import sqlite3

from common_func.db_manager import DBManager
from common_func.msvp_common import is_number
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from common_func.ms_constant.number_constant import NumberConstant


def get_sys_cpu_usage_data(db_path: str, table_name: str, configs: dict) -> tuple:
    """
    get system cpu usage
    """
    conn, curs = DBManager.check_connect_db_path(db_path)
    if not (conn and curs):
        return MsvpConstant.MSVP_EMPTY_DATA
    try:
        if not DBManager.judge_table_exist(curs, table_name):
            return MsvpConstant.MSVP_EMPTY_DATA
    except sqlite3.Error:
        return MsvpConstant.MSVP_EMPTY_DATA
    else:
        data = _sys_usage_data(curs, table_name)
        return configs.get(StrConstant.CONFIG_HEADERS), data, len(data)
    finally:
        DBManager.destroy_db_connect(conn, curs)


def _sys_usage_data(curs: any, table_name: str) -> list:
    """
    get sys cpu data
    """
    sql = "select cputype, sum(user), sum(sys), sum(iowait), sum(irq), sum(soft), sum(idle), " \
          "sum(user+nice+sys+idle+iowait+irq+soft+steal+guest+gnice) " \
          "from {} where cputype != '' group by cputype".format(table_name)
    result_data = DBManager.fetch_all_data(curs, sql)
    data_list = []
    for data in result_data:
        total_size = data[7]
        if total_size == 0:
            continue
        cpu_type = data[0]
        user_ratio = StrConstant.ACCURACY % (data[1] / total_size * NumberConstant.PERCENTAGE)
        sys_ratio = StrConstant.ACCURACY % (data[2] / total_size * NumberConstant.PERCENTAGE)
        io_wait_ratio = StrConstant.ACCURACY % (data[3] / total_size * NumberConstant.PERCENTAGE)
        irq_ratio = StrConstant.ACCURACY % (data[4] / total_size * NumberConstant.PERCENTAGE)
        soft_ratio = StrConstant.ACCURACY % (data[5] / total_size * NumberConstant.PERCENTAGE)
        idle_ratio = StrConstant.ACCURACY % (data[6] / total_size * NumberConstant.PERCENTAGE)
        data_list.append([cpu_type, user_ratio, sys_ratio, io_wait_ratio, irq_ratio, soft_ratio, idle_ratio])
    return data_list


def get_process_cpu_usage(db_path: str, table_name: str, configs: dict) -> tuple:
    """
    get process cpu usage
    """
    conn, curs = DBManager.check_connect_db_path(db_path)
    if not (conn and curs):
        return MsvpConstant.MSVP_EMPTY_DATA
    try:
        if not DBManager.judge_table_exist(curs, table_name):
            return MsvpConstant.MSVP_EMPTY_DATA
    except sqlite3.Error:
        return MsvpConstant.MSVP_EMPTY_DATA
    else:
        data = _proc_usage_data(curs, table_name)
        return configs.get(StrConstant.CONFIG_HEADERS), data, len(data)
    finally:
        DBManager.destroy_db_connect(conn, curs)


def _proc_usage_data(curs: any, table_name: str) -> list:
    """
    get process cpu usage data
    """
    proc_sql = \
        "select pid, process_name, 100*sum((utime+stime+cutime+cstime)/sys_usage)/count(*) " \
        "from {} group by pid order by 100*sum((utime+stime+cutime+cstime)/sys_usage)/count(*)" \
        " desc".format(table_name)
    proc_data = DBManager.fetch_all_data(curs, proc_sql)
    return _format_cpu_usage_data(proc_data)


def _format_cpu_usage_data(result_data: list) -> list:
    """
    format for 6 decimal places.
    :param result_data: result_data
    :return: tuple for cpu usage data.
    """
    for i, data in enumerate(result_data):
        result_list_data = list(data)
        if is_number(str(result_list_data[-1])):
            result_list_data[-1] = StrConstant.ACCURACY % result_list_data[-1]
        result_data[i] = tuple(result_list_data)
    return result_data
