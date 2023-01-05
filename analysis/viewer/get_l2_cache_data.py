#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.

import sqlite3

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.get_table_data import GetTableData
from common_func.msvp_constant import MsvpConstant
from common_func.utils import Utils


def get_l2_cache_data(db_path: str, table_name: str, unused_cols: list) -> tuple:
    """
    get l2 cache data
    """
    conn, cursor = DBManager.check_connect_db_path(db_path)
    if not (conn and cursor) or not DBManager.judge_table_exist(cursor, table_name):
        return MsvpConstant.MSVP_EMPTY_DATA
    used_cols = DBManager.get_filtered_table_headers(cursor, table_name, *unused_cols)
    data = DBManager.fetch_all_data(cursor, "select {} from {}".format(",".join(used_cols), table_name))
    if isinstance(conn, sqlite3.Connection):
        conn.close()
    modify_l2_cache_headers(used_cols)
    return used_cols, data, len(data)


def get_l2_cache_sample_data(db_path: str, table_name: str, header: list):
    conn, cursor = DBManager.check_connect_db_path(db_path)
    if not (conn and cursor) or not DBManager.judge_table_exist(cursor, table_name):
        return MsvpConstant.MSVP_EMPTY_DATA
    data = DBManager.fetch_all_data(cursor, "select * from {}".format(table_name))
    DBManager.destroy_db_connect(conn, cursor)
    return header, data, len(data)


def modify_l2_cache_headers(headers: list) -> None:
    """
    modify l2 cache headers
    """
    for index, header in enumerate(headers):
        headers[index] = " ".join(Utils.generator_to_list(letter.capitalize() for letter in header.split("_")))


def add_op_name(l2_cache_header: list, l2_cache_data: list, op_dict: dict) -> bool:
    """
    add operator name
    """
    if not {"Task Id", "Stream Id"}.issubset(l2_cache_header):
        return False
    task_id_inx = l2_cache_header.index("Task Id")
    stream_id_inx = l2_cache_header.index("Stream Id")
    for index, sub in enumerate(l2_cache_data):
        key = "{}-{}".format(sub[task_id_inx], sub[stream_id_inx])  # key is task_id-stream_id
        tmp = list(l2_cache_data[index])
        tmp.append(op_dict[key] if key in op_dict else Constant.NA)
        l2_cache_data[index] = tmp
    return True
