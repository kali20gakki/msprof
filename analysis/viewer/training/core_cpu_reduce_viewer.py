"""
This script is used to export tracing graph fro ai core, ai cpu, all reduce.
Copyright Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
"""

import json
import logging
import os
import sqlite3
from collections import OrderedDict

from analyzer.op_common_function import OpCommonFunc
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.common import CommonConstant
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import is_number
from common_func.path_manager import PathManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from viewer.aicpu_viewer import ParseAiCpuData
from viewer.memory_copy.memory_copy_viewer import MemoryCopyViewer


class CoreCpuReduceViewer:
    """
    get tracing graph for ai core,ai cpu,all reduce
    """
    # task scheduler, ai cpu, all reduce pid value
    TRACE_PID_MAP = {
        TraceViewHeaderConstant.PROCESS_TASK: 0,
        TraceViewHeaderConstant.PROCESS_AI_CPU: 1,
        TraceViewHeaderConstant.PROCESS_ALL_REDUCE: 2
    }

    @staticmethod
    def get_op_names_and_task_type(sql_path: str) -> tuple:
        """
        get op_name and task_type value
        """
        op_names = {}
        task_types = {}
        sql = "SELECT op_name, stream_id, task_id, " \
              "task_type, batch_id FROM {} ".format(DBNameConstant.TABLE_GE_TASK)
        conn_ge, cur_ge = DBManager.check_connect_db_path(
            os.path.join(sql_path, DBNameConstant.DB_GE_INFO))
        if conn_ge and cur_ge:
            if not DBManager.judge_table_exist(cur_ge, DBNameConstant.TABLE_GE_TASK):
                return op_names, task_types
            ge_datas = DBManager.fetch_all_data(cur_ge, sql)
            for ge_data in ge_datas:
                ge_data_key = "_".join(list(map(str, [ge_data[1], ge_data[2], ge_data[3], ge_data[-1]])))
                op_names[ge_data_key] = ge_data[0]
                task_types[ge_data_key] = ge_data[3]
        DBManager.destroy_db_connect(conn_ge, cur_ge)
        return op_names, task_types

    @staticmethod
    def get_total_cycle(sql_path: str) -> tuple:
        """
        get total_cycle value
        """
        total_cycle_data = {}
        total_time_data = {}
        conn_ge, cur_ge = DBManager.check_connect_db_path(
            os.path.join(sql_path, DBNameConstant.DB_RUNTIME))
        if conn_ge and cur_ge:
            if not DBManager.judge_table_exist(cur_ge, DBNameConstant.TABLE_METRICS_SUMMARY):
                return total_cycle_data, total_time_data
            total_cycles = DBManager.fetch_all_data(cur_ge, CoreCpuReduceViewer._get_aicore_sql(sql_path))
            total_cycles = OpCommonFunc.deal_batch_id(stream_index=1, task_index=2, merge_data=total_cycles)
            for total_cycle in total_cycles:
                total_data_key = "_".join(
                    [str(total_cycle[1]), str(total_cycle[2]), Constant.TASK_TYPE_AI_CORE, str(total_cycle[-1])])
                total_cycle_data[total_data_key] = total_cycle[0]
                total_time_data[total_data_key] = total_cycle[3]
        DBManager.destroy_db_connect(conn_ge, cur_ge)
        return total_cycle_data, total_time_data

    @staticmethod
    def _get_aicore_sql(sql_path: str) -> str:
        sql = "SELECT total_cycles, stream_id, task_id, total_time FROM {} ".format(
            DBNameConstant.TABLE_METRICS_SUMMARY)
        if not os.path.exists(os.path.join(sql_path, DBNameConstant.DB_GE_INFO)):
            sql = "SELECT total_cycles, stream_id, task_id FROM {} ".format(
                DBNameConstant.TABLE_METRICS_SUMMARY)
        return sql

    @staticmethod
    def _get_trace_one_device(trace_curs: any, device_id: int, iter_id: int, model_id: int) -> list:
        """
        Select date from traing trace limited by count and sort
        :param trace_curs: curs related to trace_db
        :param device_id: device id
        :param iter_id: iter id
        :param model_id: model id
        :return: training trace data
        """
        result = []
        if trace_curs:
            if ProfilingScene().is_step_trace():
                sql = "select iteration_id, FP_start, BP_end, iteration_end, iteration_time, " \
                      "fp_bp_time, grad_refresh_bound, data_aug_bound from {0} " \
                      "where device_id=? and iteration_id=? and model_id=?".format(DBNameConstant.TABLE_TRAINING_TRACE)
                result = DBManager.fetch_all_data(trace_curs, sql, (device_id, iter_id, model_id))
            elif ProfilingScene().is_training_trace():
                sql = "select iteration_id, FP_start, BP_end, iteration_end, iteration_time, " \
                      "fp_bp_time, grad_refresh_bound, data_aug_bound from {0} " \
                      "where device_id=? and iteration_id=?".format(DBNameConstant.TABLE_TRAINING_TRACE)
                result = DBManager.fetch_all_data(trace_curs, sql, (device_id, iter_id))

        return result

    @staticmethod
    def _get_reduce(trace_curs: any, device_id: str, iteration_end: int) -> list:
        """
        Select date from all_reduce table with specific ids.
        """
        result = []
        if trace_curs:
            sql = "select start, end-start from {0} " \
                  "where device_id=? and iteration_end=?" \
                .format(DBNameConstant.TABLE_ALL_REDUCE)
            result = DBManager.fetch_all_data(trace_curs, sql, (device_id, iteration_end))
        return result

    @classmethod
    def get_meta_trace_data(cls: any, name: str, tid: any = None) -> list:
        """
        get meta trace data
        """
        pid = cls.TRACE_PID_MAP.get(name)
        meta_data = [["process_name", pid, InfoConfReader().get_json_tid_data(), name]]
        if name == TraceViewHeaderConstant.PROCESS_TASK:
            meta_data.extend(["thread_name", pid, tid_value,
                              "Stream {}".format(tid_value)] for tid_value in tid)
        result = TraceViewManager.metadata_event(meta_data)
        return result

    @classmethod
    def get_task_time_data(cls: any, params: dict) -> str:
        """
        get ai core, ai cpu and all reduce data.
        :return: result
        """

        trace_data = []
        device_id = params.get('device_id')
        iter_id = params.get('iter_id')
        result_dir = params.get('result_dir')
        model_id = params.get('model_id')

        # add memory copy data
        memory_copy_viewer = MemoryCopyViewer(result_dir)
        trace_data_memcpy = memory_copy_viewer.get_memory_copy_timeline()

        trace_data_sql = \
            cls._get_task_scheduler_data(sql_path=PathManager.get_sql_dir(result_dir))
        trace_data_job = \
            cls._get_ai_cpu_data(iter_id, model_id, job_path=result_dir)

        trace_data_result = \
            cls._get_all_reduce_data(device_id, iter_id, model_id, result_dir=result_dir)
        trace_data.extend(trace_data_memcpy)
        trace_data.extend(trace_data_sql)
        trace_data.extend(trace_data_job)
        trace_data.extend(trace_data_result)
        return json.dumps(trace_data)

    @classmethod
    def _get_task_scheduler_data(cls: any, sql_path: str) -> list:
        result_data = []
        db_path = os.path.join(sql_path, DBNameConstant.DB_AICORE_OP_SUMMARY)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not conn or not curs or not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_SUMMARY_TASK_TIME):
            return result_data
        sql = "SELECT stream_id, task_id, start_time, duration_time, task_type, index_id, batch_id " \
              "FROM {table_name} WHERE start_time>=0 and duration_time>=0" \
            .format(table_name=DBNameConstant.TABLE_SUMMARY_TASK_TIME)
        task_scheduler_data = DBManager.fetch_all_data(curs, sql)
        DBManager.destroy_db_connect(conn, curs)
        tid_values = {hwts_task_data[0] for hwts_task_data in task_scheduler_data}
        result_data = cls.get_meta_trace_data(TraceViewHeaderConstant.PROCESS_TASK, tid_values)
        try:
            if task_scheduler_data:
                task_trace_datas = \
                    cls._format_task_trace_data(task_scheduler_data, sql_path)
                result_data.extend(TraceViewManager.time_graph_trace(
                    TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, task_trace_datas))
        except (TypeError, IndexError, ValueError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return result_data
        return result_data

    @classmethod
    def _format_task_trace_data(cls: any, sql_datas: list, sql_path: str) -> list:
        trace_data = []
        op_names, task_types = cls.get_op_names_and_task_type(sql_path)
        total_cycle, total_time = cls.get_total_cycle(sql_path)
        for sql_data in sql_datas:
            # key: stream_id task_id task_type batch_id
            _key_for_ops = "_".join([str(sql_data[0]), str(sql_data[1]), str(sql_data[-3]), str(sql_data[-1])])
            total_time_value = cls._get_task_trace_value(_key_for_ops, total_time)
            if is_number(total_time_value):
                total_time_value = round(total_time_value, CommonConstant.ROUND_SIX)
            trace_data_args = OrderedDict(
                [("Task Type", cls._get_task_trace_value(_key_for_ops, task_types, Constant.TASK_TYPE_OTHER)),
                 ("Stream Id", sql_data[0]),
                 ("Task Id", sql_data[1]),
                 ("Aicore Time(ms)", total_time_value),
                 ("Total Cycle", cls._get_task_trace_value(_key_for_ops, total_cycle))
                 ])
            trace_data_pice = [cls._get_task_trace_value(_key_for_ops, op_names), NumberConstant.TASK_TIME_PID,
                               sql_data[0],
                               float(sql_data[2]) / DBManager.NSTOUS,
                               int(sql_data[3]) / DBManager.NSTOUS if sql_data[3] > 0 else 0,
                               trace_data_args]
            trace_data.append(trace_data_pice)
        return trace_data

    @classmethod
    def _get_task_trace_value(cls: any, _key_for_ops: str, data: dict, default_value: str = Constant.NA) -> any:
        return data.get(_key_for_ops, default_value)

    @classmethod
    def _get_ai_cpu_data(cls: any, *args: dict, job_path: str = None) -> list:
        iter_id, model_id = args
        ai_cpu_datas = []
        dir_path = job_path
        _, ai_cpu_data = ParseAiCpuData.analysis_aicpu(dir_path, iter_id, model_id)
        result_data = []
        if ai_cpu_data:
            result_data.extend(cls.get_meta_trace_data(TraceViewHeaderConstant.PROCESS_AI_CPU))
            for each_data in ai_cpu_data:
                compute_t = each_data[2] * NumberConstant.MS_TO_US if isinstance(each_data[2], float) else Constant.NA
                memcpy_t = each_data[3] * NumberConstant.MS_TO_US if isinstance(each_data[3], float) else Constant.NA
                ai_cpu_datas.append(
                    (each_data[1], cls.TRACE_PID_MAP.get(TraceViewHeaderConstant.PROCESS_AI_CPU, 0),
                     InfoConfReader().get_json_tid_data(),
                     float(each_data[0]) * NumberConstant.MS_TO_US,
                     float(each_data[4]) * NumberConstant.MS_TO_US,
                     OrderedDict([("Task Type", "AI_CPU"),
                                  ("Compute Time(us)", compute_t),
                                  ("Memcpy Time(us)", memcpy_t),
                                  ("Task Time(us)", float(each_data[4]) * NumberConstant.MS_TO_US),
                                  ("Dispatch Time(us)", float(each_data[5]) * NumberConstant.MS_TO_US),
                                  ("Total Time(us)", float(each_data[6]) * NumberConstant.MS_TO_US)])))
            result_data.extend(
                TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, ai_cpu_datas))
        return result_data

    @classmethod
    def _get_all_reduce_data(cls: any, *args: dict, result_dir: str) -> list:
        device_id, iter_id, model_id = args
        sql_path = PathManager.get_sql_dir(result_dir)
        result_data = []
        trace_conn, trace_curs = DBManager.check_connect_db_path(
            os.path.join(sql_path, DBNameConstant.DB_TRACE))
        if not trace_conn or not trace_curs or \
                not DBManager.judge_table_exist(trace_curs, DBNameConstant.TABLE_TRAINING_TRACE) or \
                not DBManager.judge_table_exist(trace_curs, DBNameConstant.TABLE_ALL_REDUCE):
            return result_data

        result_data = cls.get_meta_trace_data(TraceViewHeaderConstant.PROCESS_ALL_REDUCE)
        all_reduce_datas = \
            cls._format_all_reduce_data(trace_curs, device_id, iter_id, model_id)
        result_data.extend(
            TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, all_reduce_datas))
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        return result_data

    @classmethod
    def _format_all_reduce_data(cls: any, trace_curs: any, device_id: int, iter_id: int, model_id: int) -> list:
        """
        get all reduce trace data
        :param trace_curs: curs related to trace_db
        :param device_id: device id
        :param iter_id: iter id
        :param model_id: model id
        :return: all_reduce_data
        """
        all_reduce_datas = []
        hwts_freq = InfoConfReader().get_freq(StrConstant.HWTS)

        for training_trace_data in cls._get_trace_one_device(trace_curs, device_id, iter_id, model_id):
            if not training_trace_data:
                continue
            all_reduces = cls._get_reduce(trace_curs, device_id, training_trace_data[3])
            for index, all_reduce in enumerate(all_reduces):
                all_reduce_time = 0
                if not NumberConstant.is_zero(hwts_freq):
                    all_reduce_time = all_reduce[1] / hwts_freq * NumberConstant.MICRO_SECOND
                all_reduce_datas.append(
                    ("AR {}".format(index), cls.TRACE_PID_MAP.get(TraceViewHeaderConstant.PROCESS_ALL_REDUCE, ""),
                     InfoConfReader().get_json_tid_data(),
                     InfoConfReader().time_from_syscnt(all_reduce[0], time_fmt=NumberConstant.MICRO_SECOND),
                     all_reduce_time,
                     OrderedDict([("Reduce Duration(us)", all_reduce_time)])))
        return all_reduce_datas
