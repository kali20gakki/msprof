#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from collections import OrderedDict

from common_func.data_manager import DataManager
from common_func.db_manager import DBManager
from common_func.msvp_common import is_number
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from common_func.platform.chip_manager import ChipManager
from common_func.utils import Utils
from viewer.ai_core_op_report import AiCoreOpReport


def get_core_sample_data(result_dir: str, db_name: str, device_id: str, params: dict) -> tuple:
    """
    get AIC/AIV sample-based data
    """
    conn, curs = DBManager.check_connect_db(result_dir, db_name.format(device_id))
    if not (conn and curs):
        return MsvpConstant.MSVP_EMPTY_DATA
    if params.get(StrConstant.CORE_DATA_TYPE) == StrConstant.AI_CORE_PMU_EVENTS or \
            params.get(StrConstant.CORE_DATA_TYPE) == StrConstant.AI_VECTOR_CORE_PMU_EVENTS:
        headers, data = get_output_event_counter(curs)
        DBManager.destroy_db_connect(conn, curs)
        return headers, data, len(data)
    DBManager.destroy_db_connect(conn, curs)
    return MsvpConstant.MSVP_EMPTY_DATA


def _get_output_event_counter(result: list, core_id: str) -> list:
    tmp = OrderedDict()
    for i in result:
        if i[0] in tmp:
            tmp[i[0]].append(float(StrConstant.ACCURACY % i[-1]) if is_number(str(i[-1])) else i[-1])
        else:
            tmp[i[0]] = [float(StrConstant.ACCURACY % i[-1]) if is_number(str(i[-1])) else i[-1]]
    remove_redundant(tmp)
    headers = ["Core ID"] + list(tmp.keys())
    headers = AiCoreOpReport.delete_special_tag(headers)
    result = [Utils.generator_to_list("Core{}".format(i[0]) for i in core_id) + ["Average"]]
    try:
        result.extend((value + [float(StrConstant.ACCURACY % (sum(value) / len(value)))]) for value in tmp.values())
    except ZeroDivisionError:
        return [], []
    else:
        # reverse rows and columns
        result = Utils.generator_to_list(list(x) for x in zip(*result))
        DataManager.add_memory_bound(headers, result)
        return headers, result
    finally:
        pass


def get_output_event_counter(cursor: any) -> tuple:
    """
    get ai core event count data
    """
    if cursor is None:
        return [], []
    is_exist = DBManager.judge_table_exist(cursor, "MetricSummary")
    if not is_exist:
        return [], []
    sql_result = "select metric, value from MetricSummary where metric not like " \
                 "'%total_time%' and value is not null;"
    sql_core_id = "select distinct(coreid) " \
                  "from MetricSummary " \
                  "where value is not null order by coreid;"
    result = DBManager.fetch_all_data(cursor, sql_result)
    core_id = DBManager.fetch_all_data(cursor, sql_core_id)

    if not result or not core_id:
        return [], []
    return _get_output_event_counter(result, core_id)


def remove_redundant(headers: list) -> None:
    """
    delete unnecessary headers
    """
    if not ChipManager().is_chip_v1() and \
            "ub_read_bw_mte(GB/s)" in headers and \
            "ub_write_bw_mte(GB/s)" in headers:
        headers.pop("ub_read_bw_mte(GB/s)")
        headers.pop("ub_write_bw_mte(GB/s)")

    if not ChipManager().is_chip_v1() and \
            "l2_read_bw(GB/s)" in headers and \
            "l2_write_bw(GB/s)" in headers:
        headers.pop("l2_read_bw(GB/s)")
        headers.pop("l2_write_bw(GB/s)")
