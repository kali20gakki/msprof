#!usr/bin/env python
# coding:utf-8
"""
This script is used to report ai core operator summary data from db.
Copyright Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
"""

import logging
import sqlite3
from collections import deque

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.common import CommonConstant
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import add_aicore_units
from common_func.msvp_common import read_cpu_cfg
from common_func.msvp_constant import MsvpConstant
from common_func.utils import Utils
from viewer.ge_info_report import get_ge_hash_dict
from viewer.ge_info_report import get_ge_model_name_dict
from viewer.runtime_report import add_mem_bound


class AiCoreOpReport:
    """
    report ai core op data
    """
    TASK_TIME_TABLE = "task_time"
    GE_SUMMARY_TABLE = "ge_summary"
    AI_CORE_UNUSED_COLS = ["job_id", "host_id", "device_id", "task_id", "stream_id", "index_id",
                           "model_id", "overflow", "overflowed_cycles", "device_id", "batch_id"]
    TENSOR_HEADERS = ["Input Shapes", "Input Data Types", "Input Formats",
                      "Output Shapes", "Output Data Types", "Output Formats"]
    OPERATOR_UNUSED_HEADERS = ["Model Name", "Infer ID"]
    HEADERS_WITH_NO_GE_DATA = ["Op Name", "OP Type", "Block Dim"]
    START_TIME_INDEX = 8
    MODEL_NAME_INDEX = 0
    INFER_ID_INDEX = 4
    AI_CPU_TABLE = "ai_cpu_datas"

    @staticmethod
    def _union_task_ge_ai_core_data(data: list, ai_core_group_dict: dict) -> list:
        union_data = []
        if not ai_core_group_dict or not data:
            return data
        for datum in data:
            # 2 stream id; 1 task id of datum

            ai_core_queue = ai_core_group_dict.get((datum[2], datum[1]), deque([]))
            if not ai_core_queue:
                logging.warning("Losing ai core data of stream %d, task %d", datum[2], datum[1])
                continue
            ai_core_datum = ai_core_queue.popleft()
            union_data.append(datum + ai_core_datum)

        for stream_task, data_queue in ai_core_group_dict.items():
            if data_queue:
                logging.warning("Losing ge or task time data of stream %d, task %d", stream_task[0], stream_task[1])

        return union_data

    @staticmethod
    def _count_num(table: str, curs: any) -> int:
        """
        count number
        """
        sql = "select count(*) from {}".format(table)
        return curs.execute(sql).fetchone()[0]

    @staticmethod
    def _get_used_headers(curs: any, table_name: str, unused_headers: list) -> list:
        """
        get used headers
        """
        all_headers = []
        for sub in DBManager.fetch_all_data(curs, "PRAGMA table_info({})".format(table_name)):
            all_headers.append(sub[1])
        return [sub for sub in all_headers if sub not in unused_headers]

    @staticmethod
    def _get_ai_core_table_sql(ai_core_used_headers: list) -> str:
        """
        get union sql statement from ai core tables
        """
        used_headers = ",".join(ai_core_used_headers)
        return "select {1}, task_id, stream_id from {0}".format(DBNameConstant.TABLE_SUMMARY_METRICS,
                                                                used_headers)

    @staticmethod
    def _get_ai_core_float_cols(columns: list) -> list:
        """
        get ai core columns with float types
        """
        ai_core_events_map = read_cpu_cfg("ai_core", "event2metric")
        all_float_cols = []
        ai_core_float_cols = Utils.generator_to_list(sub for sub in columns)
        if isinstance(ai_core_events_map, dict):
            for val in ai_core_events_map.values():
                all_float_cols.append(val.replace("(GB/s)", ""))
                all_float_cols.append(val.replace("_ratio", "_time"))
            all_float_cols.append("total_time")
            for index, col in enumerate(columns):
                if col in all_float_cols:
                    # keep six decimal places for ai core float data
                    ai_core_float_cols[index] = "round({0}, {1})".format(col, NumberConstant.DECIMAL_ACCURACY)
                    if col.find("_time") != -1:
                        ai_core_float_cols[index] = "round({0}*{1}, {2})".format(col, NumberConstant.NS_TO_US,
                                                                                 NumberConstant.DECIMAL_ACCURACY)
        return ai_core_float_cols

    @staticmethod
    def _add_memory_bound(headers: list, data: list) -> None:
        """
        add memory bound data
        """
        if "mac_ratio" in headers and "vec_ratio" in headers and "mte2_ratio" in headers:
            mte2_index = headers.index("mte2_ratio")
            vec_index = headers.index("vec_ratio")
            mac_index = headers.index("mac_ratio")
            headers.append("memory_bound")
            for index, row in enumerate(data):
                data[index] = add_mem_bound(list(row), vec_index, mac_index, mte2_index)

    @classmethod
    def get_op_summary_data(cls: any, project_path: str, db_path: str, iter_id: int, configs: dict) -> tuple:
        """
        get op summary data
        :param project_path: sqlite file path
        :param iter_id: iteration id
        :param configs: info config
        :return: headers and data
        """
        ai_core_data = cls.get_ai_core_op_summary_data(project_path, db_path, iter_id, configs)
        data = cls.get_ai_cpu_op_summary_data(project_path, db_path, iter_id, ai_core_data, configs)
        return data

    @classmethod
    def get_ai_cpu_op_summary_data(cls: any, project_path: str, db_path: str, *args: any) -> tuple:
        """
        get ai cpu op summary data
        :param project_path: project path
        :param db_path: database path
        :return: headers and data
        """
        iter_id, data, configs = args
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not cls._check_ai_cpu_data(conn, curs):
            return data
        try:
            data = cls.get_ai_cpu_data(project_path, curs, iter_id, data, configs)
            return data
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as op_err:
            logging.error(str(op_err), exc_info=Constant.TRACE_BACK_SWITCH)
            return data
        finally:
            DBManager.destroy_db_connect(conn, curs)

    @classmethod
    def get_ai_cpu_data(cls: any, project_path: str, curs: any, *args: any) -> tuple:
        """
        ai cpu metric value is N/A
        """
        iter_id, data, configs = args
        union_sql = cls._get_ai_cpu_sql(curs)
        ai_cpu_datas = DBManager.fetch_all_data(curs, union_sql, ('{0}'.format(Constant.TASK_TYPE_AI_CPU),))
        if not ai_cpu_datas:
            return data
        if not ProfilingScene().is_operator():
            ai_cpu_datas = cls._update_model_name_and_infer_id(project_path, ai_cpu_datas)
        if not data[1]:
            headers = cls.get_op_header(configs)
            return headers, ai_cpu_datas, len(ai_cpu_datas)

        op_data = data[1]
        if len(op_data[0]) > len(ai_cpu_datas[0]):
            for index, ai_cpu_data in enumerate(ai_cpu_datas):
                ai_cpu_data += (Constant.NA,) * (len(op_data[0]) - len(ai_cpu_data))
                ai_cpu_datas[index] = list(ai_cpu_data)
        op_data.extend(ai_cpu_datas)
        op_data.sort(key=lambda x: x[cls.START_TIME_INDEX])
        return data[0], op_data, len(op_data)

    @classmethod
    def get_op_header(cls: any, configs: dict) -> list:
        """
        get op summary header
        :param configs: to get headers
        :return: headers
        """
        headers = configs.get(StrConstant.CONFIG_HEADERS)
        if not ProfilingScene().is_operator():
            return headers
        for head in cls.OPERATOR_UNUSED_HEADERS:
            if head in headers:
                headers.remove(head)
        return headers

    @classmethod
    def clear_no_ge_data_headers(cls: any, headers: list) -> None:
        i = 0
        while i < len(headers):
            if headers[i] in cls.HEADERS_WITH_NO_GE_DATA:
                headers.remove(headers[i])
                continue
            i += 1

    @classmethod
    def _check_ai_cpu_data(cls: any, conn: any, curs: any) -> bool:
        """
        check ai cpu sqlite data
        Return: True or False
        """
        if not (conn and curs):
            return False
        if not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_SUMMARY_TASK_TIME):
            return False
        if not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_SUMMARY_GE):
            return False
        return True

    @classmethod
    def _check_op_summary_table_no_op_scene(cls: any, conn: any, curs: any) -> bool:
        if not (conn and curs) or not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_SUMMARY_TASK_TIME):
            return False
        return True

    @classmethod
    def _get_op_summary_data(cls: any, project_path: str, curs: any, headers: list, iter_id: int) -> tuple:
        union_sql, headers = cls._get_sql_and_headers(curs, headers)
        ai_core_group_dict, headers = cls._get_aicore_data(curs, headers)
        data = DBManager.fetch_all_data(curs, union_sql, ('{0}'.format(Constant.TASK_TYPE_AI_CPU),))
        if not data:
            return [], []
        data = cls._union_task_ge_ai_core_data(data, ai_core_group_dict)
        data = cls._update_op_name_from_hash(project_path, data)
        if not ProfilingScene().is_operator():
            data = cls._update_model_name_and_infer_id(project_path, data)
        cls._add_memory_bound(headers, data)
        headers = add_aicore_units(headers)
        return data, headers

    @classmethod
    def get_ai_core_op_summary_data(cls: any, project_path: str, db_path: str, iter_id: int, configs: dict) -> tuple:
        """
        get ai core op summary data
        :param project_path:
        :param db_path:
        :param iter_id:
        :param configs:
        :return:
        """
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not cls._check_op_summary_table_no_op_scene(conn, curs):
            return MsvpConstant.MSVP_EMPTY_DATA
        headers = cls.get_op_header(configs)
        try:
            data, headers = cls._get_op_summary_data(project_path, curs, headers, iter_id)
            return headers, data, len(data)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as op_err:
            logging.error(str(op_err), exc_info=Constant.TRACE_BACK_SWITCH)
            return MsvpConstant.MSVP_EMPTY_DATA
        finally:
            DBManager.destroy_db_connect(conn, curs)

    @classmethod
    def _update_model_name_and_infer_id(cls: any, project_path: str, ai_core_data: list) -> list:
        model_dict = get_ge_model_name_dict(project_path)
        result_data = []
        for _data in ai_core_data:
            _data = list(_data)
            model_name = model_dict.get(_data[0], Constant.NA)
            _data.insert(cls.MODEL_NAME_INDEX, model_name)
            result_data.append(_data)
        return result_data

    @classmethod
    def _update_op_name_from_hash(cls: any, project_path: str, ai_core_data: list) -> list:
        hash_dict = get_ge_hash_dict(project_path)
        result_data = []
        if not hash_dict:
            return ai_core_data
        for _data in ai_core_data:
            _data = list(_data)
            _data[3] = hash_dict.get(_data[3], _data[3])  # op_name
            result_data.append(_data)
        return result_data

    @classmethod
    def _group_by_stream_task(cls: any, ai_core_data: list) -> dict:
        ai_core_group_dict = {}
        for ai_core_datum in ai_core_data:
            # -1 stream id; -2 task id
            ai_core_group_value = ai_core_group_dict.setdefault((ai_core_datum[-1], ai_core_datum[-2]), deque([]))
            ai_core_group_value.append(ai_core_datum[:-2])
        return ai_core_group_dict

    @classmethod
    def _get_aicore_data(cls: any, curs: any, headers: list) -> tuple:
        if DBManager.judge_table_exist(curs, DBNameConstant.TABLE_SUMMARY_METRICS) \
                and cls._count_num(DBNameConstant.TABLE_SUMMARY_METRICS, curs):
            ai_core_headers = cls._get_used_headers(curs,
                                                    DBNameConstant.TABLE_SUMMARY_METRICS,
                                                    cls.AI_CORE_UNUSED_COLS)
            ai_core_used_cols = cls._get_ai_core_float_cols(ai_core_headers)
            ai_core_data = DBManager.fetch_all_data(curs, cls._get_ai_core_table_sql(ai_core_used_cols))
            ai_core_group_dict = cls._group_by_stream_task(ai_core_data)
            headers += ai_core_headers
            return ai_core_group_dict, headers
        return {}, headers

    @classmethod
    def _get_two_table_union_sql(cls: any) -> str:
        """
        get union sql statement from task time and ge tables
        """
        batch_limit = "and {0}.batch_id={1}.batch_id" \
            .format(DBNameConstant.TABLE_SUMMARY_TASK_TIME, DBNameConstant.TABLE_SUMMARY_GE)
        index_info = "{0}.index_id, ".format(DBNameConstant.TABLE_SUMMARY_TASK_TIME)
        if ProfilingScene().is_operator():
            index_info = ''
        return "select {1}.model_id, {0}.task_id, {0}.stream_id, {index_info} " \
               "op_name, {1}.op_type, {1}.task_type, start_time, duration_time/{NS_TO_US}, " \
               "wait_time/{NS_TO_US}, block_dim from {0} " \
               "inner join {1} on {0}.task_id={1}.task_id and {0}.stream_id = {1}.stream_id " \
               "and {0}.task_type = {1}.task_type " \
               "and {1}.task_type!=? {BATCH_LIMIT} order by start_time" \
            .format(DBNameConstant.TABLE_SUMMARY_TASK_TIME, DBNameConstant.TABLE_SUMMARY_GE,
                    BATCH_LIMIT=batch_limit,
                    NS_TO_US=NumberConstant.NS_TO_US,
                    index_info=index_info)

    @classmethod
    def _get_ai_cpu_sql(cls: any, curs: any) -> str:
        """
        get sql to get ai cpu data
        Return: sql
        """
        batch_limit = "and {0}.batch_id={1}.batch_id".format(
            DBNameConstant.TABLE_SUMMARY_TASK_TIME, DBNameConstant.TABLE_SUMMARY_GE)
        index_info = "{0}.index_id, ".format(DBNameConstant.TABLE_SUMMARY_TASK_TIME)
        if ProfilingScene().is_operator():
            index_info = ''
        union_sql = "select {1}.model_id, {0}.task_id, {1}.stream_id, {index_info} " \
                    "op_name, {1}.op_type, {1}.task_type, " \
                    "{0}.start_time, {0}.duration_time/{NS_TO_US}, " \
                    "{0}.wait_time/{NS_TO_US}, block_dim from {0} " \
                    "inner join {1} on {0}.task_id={1}.task_id " \
                    "and {0}.task_type = {1}.task_type " \
                    "and {0}.stream_id={1}.stream_id " \
                    "and {1}.task_type=? {BATCH_LIMIT}" \
            .format(DBNameConstant.TABLE_SUMMARY_TASK_TIME, DBNameConstant.TABLE_SUMMARY_GE,
                    BATCH_LIMIT=batch_limit,
                    NS_TO_US=NumberConstant.NS_TO_US,
                    index_info=index_info)
        if DBManager.judge_table_exist(curs, DBNameConstant.TABLE_SUMMARY_TENSOR):
            union_sql = "select {1}.model_id, {0}.task_id, {1}.stream_id, {index_info} " \
                        "op_name, {1}.op_type, {1}.task_type, " \
                        "{0}.start_time, {0}.duration_time/{NS_TO_US}, " \
                        "{0}.wait_time/{NS_TO_US}, block_dim, " \
                        "input_shapes, input_data_types, input_formats, " \
                        "output_shapes, output_data_types, output_formats " \
                        "from {0} inner join {1} on {0}.task_id={1}.task_id " \
                        "and {0}.stream_id={1}.stream_id " \
                        "and {0}.task_type = {1}.task_type " \
                        "inner join {2} on {0}.task_id={2}.task_id and {0}.stream_id={2}.stream_id " \
                        "and {1}.timestamp={2}.timestamp " \
                        "and {1}.task_type=? {BATCH_LIMIT}" \
                .format(DBNameConstant.TABLE_SUMMARY_TASK_TIME,
                        DBNameConstant.TABLE_SUMMARY_GE,
                        DBNameConstant.TABLE_SUMMARY_TENSOR,
                        BATCH_LIMIT=batch_limit,
                        NS_TO_US=NumberConstant.NS_TO_US,
                        index_info=index_info)
        return union_sql

    @classmethod
    def _get_tensor_table_sql_and_headers(cls: any, headers: list) -> tuple:
        batch_limit = "and {0}.batch_id={1}.batch_id" \
            .format(DBNameConstant.TABLE_SUMMARY_TASK_TIME,
                    DBNameConstant.TABLE_SUMMARY_GE)
        index_info = "{0}.index_id, ".format(DBNameConstant.TABLE_SUMMARY_TASK_TIME)
        if ProfilingScene().is_operator():
            index_info = ''
        sql = "select {1}.model_id, {0}.task_id, {0}.stream_id, {index_info}" \
              "{1}.op_name, {1}.op_type, {1}.task_type, " \
              "{0}.start_time, {0}.duration_time/{NS_TO_US}, {0}.wait_time/{NS_TO_US}, {1}.block_dim, " \
              "input_shapes, input_data_types, input_formats, " \
              "output_shapes, output_data_types, output_formats " \
              "from {0} inner join {2} on " \
              "{0}.task_id={2}.task_id and {0}.stream_id={2}.stream_id " \
              "and {0}.task_type = {1}.task_type " \
              "inner join {1} on {0}.task_id={1}.task_id and {0}.stream_id={1}.stream_id " \
              "and {1}.timestamp={2}.timestamp " \
              "and {1}.task_type!=? {BATCH_LIMIT} order by start_time" \
            .format(DBNameConstant.TABLE_SUMMARY_TASK_TIME,
                    DBNameConstant.TABLE_SUMMARY_GE,
                    DBNameConstant.TABLE_SUMMARY_TENSOR,
                    BATCH_LIMIT=batch_limit,
                    NS_TO_US=NumberConstant.NS_TO_US,
                    index_info=index_info)

        headers += cls.TENSOR_HEADERS
        return sql, headers

    @classmethod
    def _get_table_sql_and_headers_without_ge(cls: any, headers: list) -> tuple:
        cls.clear_no_ge_data_headers(headers)
        index_info = "{0}.index_id, ".format(DBNameConstant.TABLE_SUMMARY_TASK_TIME)
        if ProfilingScene().is_operator():
            index_info = ''
        sql = "select model_id, task_id, stream_id, {index_info} task_type, start_time, " \
              "duration_time/{NS_TO_US}, wait_time/{NS_TO_US} from {0} where " \
              "task_type!=? order by start_time" \
            .format(DBNameConstant.TABLE_SUMMARY_TASK_TIME, NS_TO_US=NumberConstant.NS_TO_US, index_info=index_info)
        return sql, headers

    @classmethod
    def _get_sql_and_headers(cls: any, curs: any, headers: list) -> tuple:
        if DBManager.judge_table_exist(curs, DBNameConstant.TABLE_SUMMARY_TENSOR):
            return cls._get_tensor_table_sql_and_headers(headers)
        if not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_SUMMARY_GE):
            return cls._get_table_sql_and_headers_without_ge(headers)

        return cls._get_two_table_union_sql(), headers


class ReportOPCounter:
    """
    class to report op counter data
    """
    OPERATOR_UNUSED_HEADERS = ["Model Name", "Infer ID"]

    @staticmethod
    def check_param(conn: any, curs: any) -> bool:
        """
        check exist of db table
        """
        if not (conn and curs) or not DBManager.judge_table_exist(curs, CommonConstant.OP_REPORT_TABLE):
            return False
        return True

    @staticmethod
    def _get_op_report_sql_operator_scene() -> str:
        sql = "select op_type, core_type, occurrences, total_time/{NS_TO_US}, " \
              "min/{NS_TO_US}, avg/{NS_TO_US}, max/{NS_TO_US}, ratio from {0} order by " \
              "total_time desc".format(CommonConstant.OP_REPORT_TABLE, NS_TO_US=NumberConstant.NS_TO_US)
        return sql

    @staticmethod
    def _get_op_report_sql_non_operator_scene() -> str:
        sql = "select model_name, op_type, core_type, occurrences, total_time/{NS_TO_US}, " \
              "min/{NS_TO_US}, avg/{NS_TO_US}, max/{NS_TO_US}, ratio from {0} " \
              "order by model_name asc, " \
              "total_time desc".format(CommonConstant.OP_REPORT_TABLE, NS_TO_US=NumberConstant.NS_TO_US)
        return sql

    @classmethod
    def _clear_unused_headers(cls: any, headers: list) -> None:
        for head in cls.OPERATOR_UNUSED_HEADERS:
            if head in headers:
                headers.remove(head)

    @classmethod
    def report_op(cls: any, db_path: str, headers: list) -> tuple:
        """
        report op counter
        :param db_path: DB path
        :param headers: table headers
        :return: headers, data, data length
        """
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not cls.check_param(conn, curs):
            return MsvpConstant.MSVP_EMPTY_DATA
        sql = cls._get_op_report_sql_non_operator_scene()
        if ProfilingScene().is_operator():
            sql = cls._get_op_report_sql_operator_scene()
            cls._clear_unused_headers(headers)
        data = DBManager.fetch_all_data(curs, sql)
        DBManager.destroy_db_connect(conn, curs)
        return headers, data, len(data)
