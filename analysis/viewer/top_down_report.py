#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import json
import logging
import sqlite3
from collections import OrderedDict

from msconfig.config_manager import ConfigManager
from common_func.common import CommonConstant
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.utils import Utils
from profiling_bean.db_dto.step_trace_dto import IterationRange
from viewer.training.core_cpu_reduce_viewer import CoreCpuReduceViewer


class TopDownData:
    """
    get top down data, include acl,ge,runtime and task scheduler data
    """

    EXPORT_ALL_TOP_DOWN_DATA = 0
    TS_SCHEDULER_LENGTH = 8
    CONVERSION_TIME = 1000

    # top down data modules.
    MODULE_ACL = "ACL"
    MODULE_GE = "GE"
    MODULE_RUNTIME = "Runtime"
    MODULE_TASK_SCHEDULER = "Task Scheduler"

    # top down data db table
    OP_SUMMARY_TASK_TIME_TABLE = "task_time"
    OP_SUMMARY_METRICS = "ai_core_metrics"

    # db table column names.
    COLUMN_RUNTIME_ITER_ID = "index_id"
    ACL_MODEL_EXECUTE = "aclmdlExecute"
    ACL_OP_EXECUTE = "aclopExecute"

    # tracing json keys.
    JSON_TRACE_TOTAL_TIME = "Total Time(ms)"

    # tracing json tid value.
    ACL_TID = 0
    GE_TID = 1
    RUNTIME_TID = 2
    TASK_TID = 3

    # api names in runtime data
    API_NAMES = [
        "KernelLaunch", "KernelLaunchWithHandleFlowCtrl",
        "KernelLaunchFlowCtrl", "rtAicpuKernelLaunchExWithArgs"
    ]

    @staticmethod
    def _dispatch_top_down_datas(top_down_datas: list) -> dict:
        dispatch_result = OrderedDict(
            [(TopDownData.MODULE_ACL, []), (TopDownData.MODULE_GE, []), (TopDownData.MODULE_RUNTIME, []),
             (TopDownData.MODULE_TASK_SCHEDULER, [])])
        for top_down_data in top_down_datas:
            dispatch_result[top_down_data[1]].append(top_down_data)
        return dispatch_result

    @staticmethod
    def _check_sql_file(conn: any, curs: any, table_name: str) -> bool:
        if not conn or not curs:
            return False
        if not DBManager.judge_table_exist(curs, table_name):
            return False
        return True

    @classmethod
    def get_top_down_data(cls: any, project_path: str, device_id: str, iter_range: IterationRange) -> tuple:
        """
        query and get top down data
        """
        logging.info("start to get top down data.")
        headers = ConfigManager.get(ConfigManager.MSPROF_EXPORT_DATA).get('ai_stack_time', 'headers').split(",")
        top_down_data = []
        for _index in range(iter_range.iteration_count):
            top_down_data.extend(
                cls._get_top_down_data_one_iter(project_path, device_id, iter_range.iteration_id + _index))
        logging.info("get top down data finish.")
        return headers, top_down_data, len(top_down_data)

    @classmethod
    def get_top_down_timeline_data(cls: any, project_path: str, device_id: str, iter_range: IterationRange) -> str:
        """
        export top down tracing data
        """
        result_data = []
        logging.info("Start to get top down tracing data.")
        meta_data = [
            [
                "process_name", InfoConfReader().get_json_pid_data(),
                InfoConfReader().get_json_tid_data(), TraceViewHeaderConstant.PROCESS_AI_STACK_TIME
            ],
            [
                "thread_name", InfoConfReader().get_json_pid_data(),
                cls.ACL_TID, TraceViewHeaderConstant.PROCESS_ACL
            ],
            [
                "thread_name", InfoConfReader().get_json_pid_data(),
                cls.GE_TID, TraceViewHeaderConstant.PROCESS_GE
            ],
            [
                "thread_name", InfoConfReader().get_json_pid_data(),
                cls.RUNTIME_TID, TraceViewHeaderConstant.PROCESS_RUNTIME
            ],
            [
                "thread_name", InfoConfReader().get_json_pid_data(),
                cls.TASK_TID, TraceViewHeaderConstant.PROCESS_TASK
            ]
        ]
        result_data.extend(TraceViewManager.metadata_event(meta_data))
        try:
            cls._export_top_down_data_by_iter(project_path, device_id, result_data, iter_range.iteration_id)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return ""
        if result_data:
            return json.dumps(result_data)
        return ""

    @classmethod
    def _get_acl_data(cls: any, project_path: str, device_id: str, iter_id: int, db_name: str) -> list:
        conn, cur = DBManager.check_connect_db(project_path, db_name)
        try:
            if conn and cur:
                # get acl data for app.
                acl_net_data = \
                    cur.execute("select api_name, start_time, (end_time-start_time) "
                                "as output_duration from acl_data "
                                "where api_name=? and device_id=? limit ?,?",
                                (cls.ACL_MODEL_EXECUTE, device_id, iter_id - 1, 1)).fetchone()
                if acl_net_data:
                    return [(iter_id, cls.MODULE_ACL) + acl_net_data]

                # get acl data for op.
                acl_op_data = \
                    cur.execute("select api_name, start_time, (end_time-start_time) "
                                "as output_duration from acl_data "
                                "where api_name=? and device_id=? limit ?,?",
                                (cls.ACL_OP_EXECUTE, device_id, iter_id - 1, 1)).fetchone()
                if acl_op_data:
                    return [(iter_id, cls.MODULE_ACL) + acl_op_data]
            return []
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return []
        finally:
            DBManager.destroy_db_connect(conn, cur)

    @classmethod
    def _get_ge_data(cls: any, project_path: str, iter_id: int, db_name: str) -> list:
        conn, cur = DBManager.check_connect_db(project_path, db_name)
        if not (conn and cur):
            return []
        if not DBManager.judge_table_exist(cur, DBNameConstant.TABLE_GE_MODEL_TIME):
            return []
        try:
            ge_data = cur.execute(
                "select input_start, (input_end-input_start) "
                "as input_duration,infer_start, (infer_end-infer_start) "
                "as infer_duration, output_start,(output_end-output_start) "
                "as output_duration from GeModelTime limit ?,?",
                (iter_id - 1, 1)).fetchone()
        except sqlite3.Error as err:
            logging.error(err)
            return []
        finally:
            DBManager.destroy_db_connect(conn, cur)
        if ge_data:
            return [(iter_id, cls.MODULE_GE, "Input", ge_data[0], ge_data[1]),
                    (iter_id, cls.MODULE_GE, "Infer", ge_data[2], ge_data[3]),
                    (iter_id, cls.MODULE_GE, "Output", ge_data[4], ge_data[5])]
        return []

    @classmethod
    def _get_runtime_data(cls: any, project_path: str, iter_id: int, db_name: str) -> list:
        conn, cur = DBManager.check_connect_db(project_path, db_name)
        if not (conn and cur):
            return []
        if not DBManager.judge_table_exist(cur, DBNameConstant.TABLE_API_CALL):
            return []
        try:
            runtime_start = cur.execute(
                "select min(entry_time) from ApiCall where api in ('{}')".format(
                    '\', \''.join(cls.API_NAMES))).fetchone()
        except sqlite3.Error as err:
            DBManager.destroy_db_connect(conn, cur)
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return []
        try:
            runtime_end = cur.execute(
                "select max(exit_time) from ApiCall where api in ('{}')".format(
                    '\', \''.join(cls.API_NAMES))).fetchone()
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return []
        finally:
            DBManager.destroy_db_connect(conn, cur)
        if runtime_start and runtime_end:
            if runtime_start[0] is not None and runtime_end[0] is not None:
                return [(iter_id, cls.MODULE_RUNTIME, Constant.NA, runtime_start[0],
                         runtime_end[0] - runtime_start[0])]
        return []

    @classmethod
    def _get_task_scheduler(cls: any, project_path: str, iter_id: int) -> list:
        conn, cur = DBManager.check_connect_db(project_path, DBNameConstant.DB_AICORE_OP_SUMMARY)
        if not (cur and conn and DBManager.judge_table_exist(cur, cls.OP_SUMMARY_TASK_TIME_TABLE)):
            return []
        try:
            # get task scheduler data from aicore summary.
            ts_start_data = cur.execute(
                "select min(start_time) from {0} "
                "where index_id=?".format(cls.OP_SUMMARY_TASK_TIME_TABLE), (iter_id,)).fetchone()
        except sqlite3.Error as err:
            DBManager.destroy_db_connect(conn, cur)
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return []
        try:
            ts_end_data = cur.execute(
                "select max(start_time), duration_time from {0} "
                "where index_id=?".format(cls.OP_SUMMARY_TASK_TIME_TABLE), (iter_id,)).fetchone()
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return []
        finally:
            DBManager.destroy_db_connect(conn, cur)
        if ts_start_data and ts_start_data[0] is not None:
            return [(iter_id, cls.MODULE_TASK_SCHEDULER, Constant.NA, ts_start_data[0],
                     ts_end_data[0] + ts_end_data[1] - ts_start_data[0])]
        return []

    @classmethod
    def _get_top_down_data_one_iter(cls: any, project_path: str, device_id: str, iter_id: int = 1) -> list:
        acl_data = cls._get_acl_data(project_path, device_id, iter_id, DBNameConstant.DB_ACL_MODULE)
        ge_data = cls._get_ge_data(project_path, iter_id, DBNameConstant.DB_GE_MODEL_TIME)
        runtime_data = cls._get_runtime_data(project_path, iter_id,
                                             DBNameConstant.DB_RUNTIME)
        ts_data = cls._get_task_scheduler(project_path, iter_id)
        top_down_data = acl_data + ge_data + runtime_data + ts_data
        return top_down_data

    @classmethod
    def _format_data(cls: any, top_down_datas: list) -> list:
        # top_down_datas: iter_id, type, op_name, start_time, duration_time, (total_time), pid, tid
        # total_time exist when type is Task Scheduler
        trace_data = []
        for top_down_data in top_down_datas:
            trace_data_args = OrderedDict([("Iter ID", top_down_data[0]),
                                           ("Start Time", top_down_data[3]),
                                           ("End Time", int(top_down_data[3])
                                            + int(top_down_data[4])),
                                           ("Duration Time(ns)", top_down_data[4])
                                           ])
            if len(top_down_data) == cls.TS_SCHEDULER_LENGTH:
                trace_data_args[cls.JSON_TRACE_TOTAL_TIME] = top_down_data[5]
            trace_data_pice = [
                top_down_data[2], top_down_data[-2], top_down_data[-1],
                int(top_down_data[3]) // cls.CONVERSION_TIME,
                int(top_down_data[4]) // cls.CONVERSION_TIME,
                trace_data_args
            ]
            trace_data.append(trace_data_pice)
        return trace_data

    @classmethod
    def _fill_top_down_trace_data(cls: any, project_path: str, device_id: str,
                                  result_data: list, top_down_datas: list) -> None:
        # top_down_datas:acl,ge include (input,infer,output), runtime,ts
        # top down data dispatch and deal by module.
        dispatch_result = cls._dispatch_top_down_datas(top_down_datas)
        cls._fill_acl_trace_data(project_path, device_id, result_data, dispatch_result)
        cls._fill_ge_trace_data(result_data, dispatch_result.get(cls.MODULE_GE))
        cls._fill_runtime_trace_data(project_path, result_data,
                                     dispatch_result.get(cls.MODULE_RUNTIME))
        cls._fill_ts_trace_data(project_path, result_data, dispatch_result.get(cls.MODULE_TASK_SCHEDULER))

    @classmethod
    def _reformat_acl_trace_data(cls: any, acl_data: list, acl_iter_data: any) -> list:
        res = []
        res.extend((acl_data[0][0], cls.MODULE_ACL))
        res.extend(acl_iter_data)
        res.extend((InfoConfReader().get_json_pid_data(),
                    cls.ACL_TID))
        return res

    @classmethod
    def _fill_acl_trace_data(cls: any, project_path: str, device_id: str, result_data: list,
                             top_down_datas: dict) -> None:
        acl_data = top_down_datas.get(cls.MODULE_ACL)
        ts_datas = top_down_datas.get(cls.MODULE_TASK_SCHEDULER)
        if acl_data and ts_datas:
            conn, cur = DBManager.check_connect_db(project_path, DBNameConstant.DB_ACL_MODULE)
            if not conn or not cur:
                return
            sql = "select api_name, start_time, (end_time-start_time) " \
                  "as output_duration from acl_data where device_id=? " \
                  "and start_time >=? and start_time<=?"
            acl_iter_datas = DBManager.fetch_all_data(cur, sql, (device_id, acl_data[0][-2],
                                                                 ts_datas[0][-2] + ts_datas[0][-1]))
            if acl_iter_datas:
                top_down_datas = Utils.generator_to_list(cls._reformat_acl_trace_data(acl_data, acl_iter_data)
                                                         for acl_iter_data in acl_iter_datas)
                trace_data = cls._format_data(top_down_datas)
                result_data.extend(TraceViewManager.time_graph_trace(
                    TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, trace_data))
            else:
                acl_data = Utils.generator_to_list(acl_item + (InfoConfReader().get_json_pid_data(),
                                                               cls.ACL_TID)
                                                   for acl_item in acl_data)
                trace_data = cls._format_data(acl_data)
                result_data.extend(TraceViewManager.time_graph_trace(
                    TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, trace_data))
            DBManager.destroy_db_connect(conn, cur)

    @classmethod
    def _reformat_ge_trace_data(cls: any, ge_item: any) -> list:
        res = []
        res.extend(ge_item)
        res.extend((InfoConfReader().get_json_pid_data(),
                    cls.GE_TID))
        return res

    @classmethod
    def _fill_ge_trace_data(cls: any, result_data: list, top_down_datas: list) -> None:
        if top_down_datas:
            top_down_datas = Utils.generator_to_list(cls._reformat_ge_trace_data(ge_item)
                                                     for ge_item in top_down_datas)
            trace_data = cls._format_data(top_down_datas)
            result_data.extend(
                TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, trace_data))

    @classmethod
    def _reformat_runtime_trace_data(cls: any, top_down_datas: list, runtime_api: any) -> list:
        res = []
        res.extend(top_down_datas[0][:2])
        res.extend((runtime_api[0], runtime_api[1], runtime_api[2],
                   InfoConfReader().get_json_pid_data(), cls.RUNTIME_TID))
        return res

    @classmethod
    def _fill_runtime_trace_data(cls: any, project_path: str, result_data: list, top_down_datas: list) -> None:
        conn, cur = DBManager.check_connect_db(project_path, DBNameConstant.DB_RUNTIME)
        if conn and cur and top_down_datas:
            sql = "select api, entry_time, exit_time-entry_time from ApiCall where " \
                  "entry_time>=? and exit_time<=?"
            runtime_apis = DBManager.fetch_all_data(cur, sql, (top_down_datas[0][3],
                                                               top_down_datas[0][3] +
                                                               top_down_datas[0][4]))
            if runtime_apis:
                runtime_trace_data = \
                    Utils.generator_to_list(cls._reformat_runtime_trace_data(top_down_datas, runtime_api)
                                            for runtime_api in runtime_apis)
                trace_data = cls._format_data(runtime_trace_data)
                result_data.extend(
                    TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, trace_data))
        DBManager.destroy_db_connect(conn, cur)

    @classmethod
    def _reformat_ts_trace_data(cls: any, top_down_datas: list, op_names: dict, total_time: dict,
                                ts_trace: any) -> list:
        # top_down_datas: iter_id, type
        # ts_trace: start_time, duration_time, stream_id, task_id, task_type, batch_id
        stream_task_key = "_".join(map(str, [ts_trace[2], ts_trace[3], ts_trace[5]]))
        res = []
        res.extend(top_down_datas[0][:2])
        res.append(op_names.get(stream_task_key, Constant.NA))
        res.extend(ts_trace[:2])  # start_time, duration_time,
        res.append(total_time.get(stream_task_key, ""))
        res.extend((InfoConfReader().get_json_pid_data(), cls.TASK_TID))
        return res

    @classmethod
    def _fill_ts_trace_data(cls: any, project_path: str, result_data: list, top_down_datas: list) -> None:
        conn, cur = DBManager.check_connect_db(project_path, DBNameConstant.DB_AICORE_OP_SUMMARY)
        if conn and cur and top_down_datas:
            if not DBManager.judge_table_exist(cur, cls.OP_SUMMARY_TASK_TIME_TABLE):
                return

            acl_query_sql = "select start_time,duration_time,stream_id,task_id,task_type,batch_id from {0} " \
                            "order by start_time".format(cls.OP_SUMMARY_TASK_TIME_TABLE)
            ts_traces = DBManager.fetch_all_data(cur, acl_query_sql)
            if ts_traces:
                op_names, _ = CoreCpuReduceViewer.get_op_names_and_task_type(PathManager.get_sql_dir(project_path))
                _, total_time = CoreCpuReduceViewer.get_total_cycle(PathManager.get_sql_dir(project_path))
                ts_trace_data = Utils.generator_to_list(
                    cls._reformat_ts_trace_data(top_down_datas, op_names, total_time, ts_trace)
                    for ts_trace in ts_traces)
                trace_data = cls._format_data(ts_trace_data)
                result_data.extend(
                    TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, trace_data))
        DBManager.destroy_db_connect(conn, cur)

    @classmethod
    def _export_top_down_data_by_iter(cls: any, project_path: str, device_id: str,
                                      result_data: list, iter_id: int) -> None:
        top_down_datas = cls._get_top_down_data_one_iter(
            project_path, device_id, iter_id)
        if top_down_datas:
            cls._fill_top_down_trace_data(
                project_path, device_id, result_data, top_down_datas)
