#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import json
import logging
import os
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
from common_func.platform.chip_manager import ChipManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.step_trace_dto import IterationRange
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
        TraceViewHeaderConstant.PROCESS_ALL_REDUCE: 2,
        TraceViewHeaderConstant.PEOCESS_ACSQ: 3
    }

    @staticmethod
    def get_op_names_and_task_type(sql_path: str) -> tuple:
        """
        get op_name and task_type value
        """
        op_names = {}
        task_types = {}
        sql = "SELECT op_name, stream_id, task_id, " \
              "task_type, batch_id FROM {} ".format(DBNameConstant.TABLE_SUMMARY_GE)
        conn_ge, cur_ge = DBManager.check_connect_db_path(
            os.path.join(sql_path, DBNameConstant.DB_AICORE_OP_SUMMARY))
        if conn_ge and cur_ge:
            if not DBManager.judge_table_exist(cur_ge, DBNameConstant.TABLE_SUMMARY_GE):
                return op_names, task_types
            ge_datas = DBManager.fetch_all_data(cur_ge, sql)
            for ge_data in ge_datas:
                ge_data_key = "_".join(list(map(str, [ge_data[1], ge_data[2], ge_data[-1]])))
                op_names[ge_data_key] = ge_data[0]
                task_types[ge_data_key] = ge_data[3]
        DBManager.destroy_db_connect(conn_ge, cur_ge)
        return op_names, task_types

    @staticmethod
    def get_task_type_from_rts(sql_path: str) -> dict:
        """
        get task type for other tasks
        """
        task_types = {}
        sql = f"select task_type,stream_id,task_id,batch_id from {DBNameConstant.TABLE_RUNTIME_TRACK}"

        conn_rts, cur_rts = DBManager.check_connect_db_path(os.path.join(sql_path, DBNameConstant.DB_RTS_TRACK))
        if conn_rts and cur_rts and DBManager.judge_table_exist(cur_rts, DBNameConstant.TABLE_RUNTIME_TRACK):
            rts_data = DBManager.fetch_all_data(cur_rts, sql)
            for _rts_data in rts_data:
                rts_data_key = "_".join(list(map(str, _rts_data[1:])))
                task_types[rts_data_key] = _rts_data[0]
        DBManager.destroy_db_connect(conn_rts, cur_rts)
        return task_types

    @staticmethod
    def get_task_type_from_stars(sql_path: str) -> dict:
        """
        get task type for other tasks
        """
        task_types = {}
        sql = f"select task_type,stream_id,task_id,batch_id from {DBNameConstant.TABLE_SUMMARY_TASK_TIME}"

        conn, cur = DBManager.check_connect_db_path(os.path.join(sql_path, DBNameConstant.DB_AICORE_OP_SUMMARY))
        if conn and cur and DBManager.judge_table_exist(cur, DBNameConstant.TABLE_SUMMARY_TASK_TIME):
            data = DBManager.fetch_all_data(cur, sql)
            for _rts_data in data:
                rts_data_key = "_".join(list(map(str, _rts_data[1:])))
                task_types[rts_data_key] = _rts_data[0]
        DBManager.destroy_db_connect(conn, cur)
        return task_types

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
            total_cycles = OpCommonFunc.deal_batch_id(stream_index=0, task_index=1, merge_data=total_cycles)
            for total_cycle in total_cycles:
                total_data_key = "_".join(
                    [str(total_cycle[0]), str(total_cycle[1]), str(total_cycle[-1])])
                total_cycle_data[total_data_key] = total_cycle[0]
                total_time_data[total_data_key] = total_cycle[3]
        DBManager.destroy_db_connect(conn_ge, cur_ge)
        return total_cycle_data, total_time_data

    @staticmethod
    def _get_aicore_sql(sql_path: str) -> str:
        total_cycles = 'total_time, total_cycles'
        if ChipManager().is_chip_v4():
            total_cycles = 'aic_total_time, aic_total_cycles, aiv_total_time, aiv_total_cycles'
        sql = "SELECT stream_id, task_id, {total_cycles} FROM {} ".format(
            DBNameConstant.TABLE_METRICS_SUMMARY, total_cycles=total_cycles)
        if not os.path.exists(os.path.join(sql_path, DBNameConstant.DB_GE_INFO)):
            sql = "SELECT stream_id, task_id, {total_cycles} FROM {} ".format(
                DBNameConstant.TABLE_METRICS_SUMMARY, total_cycles=total_cycles)
        return sql

    @staticmethod
    def _get_trace_one_device(trace_curs: any, device_id: int, iter_range: IterationRange) -> list:
        """
        Select date from traing trace limited by count and sort
        :param trace_curs: curs related to trace_db
        :param device_id: device id
        :param iter_range: iter range
        :return: training trace data
        """
        result = []
        if trace_curs:
            if ProfilingScene().is_step_trace():
                sql = "select iteration_id, FP_start, BP_end, iteration_end, iteration_time, " \
                      "fp_bp_time, grad_refresh_bound, data_aug_bound from {0} " \
                      "where device_id=? and iteration_id>=? and iteration_id<=? " \
                      "and model_id=?".format(DBNameConstant.TABLE_TRAINING_TRACE)
                result = DBManager.fetch_all_data(trace_curs, sql,
                                                  (device_id, *iter_range.get_iteration_range(), iter_range.model_id))
        return result

    @staticmethod
    def _get_reduce(trace_curs: any, device_id: str, iteration_end: int) -> list:
        """
        Select date from all_reduce table with specific ids.
        """
        result = []
        if trace_curs:
            sql = "select start, end-start from {0} " \
                  "where device_id=? and iteration_end=? and start is not null and end is not null" \
                .format(DBNameConstant.TABLE_ALL_REDUCE)
            result = DBManager.fetch_all_data(trace_curs, sql, (device_id, iteration_end))
        return result

    @classmethod
    def get_rts_track_task_type(cls: any, sql_path: str) -> dict:
        rts_task_type = cls.get_task_type_from_rts(sql_path)
        return rts_task_type if rts_task_type else cls.get_task_type_from_stars(sql_path)

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
        device_id = params.get(StrConstant.PARAM_DEVICE_ID)
        iter_range = params.get(StrConstant.PARAM_ITER_ID)
        result_dir = params.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

        # add memory copy data
        memory_copy_viewer = MemoryCopyViewer(result_dir)
        trace_data_memcpy = memory_copy_viewer.get_memory_copy_timeline()

        trace_data_sql = \
            cls._get_task_scheduler_data(sql_path=PathManager.get_sql_dir(result_dir))
        trace_data_job = \
            cls._get_ai_cpu_data(job_path=result_dir)

        trace_data_result = \
            cls._get_all_reduce_data(device_id, iter_range, result_dir=result_dir)
        trace_data.extend(trace_data_memcpy)
        trace_data.extend(trace_data_sql)
        trace_data.extend(trace_data_job)
        trace_data.extend(trace_data_result)
        return json.dumps(trace_data)

    @classmethod
    def _get_acsq_time_data(cls: any, result_dir: str):
        acsq_model_view = ViewModel(result_dir, DBNameConstant.DB_SOC_LOG, DBNameConstant.TABLE_ACSQ_TASK_TIME)
        acsq_model_view.init()
        acsq_data = acsq_model_view.get_all_data(DBNameConstant.TABLE_ACSQ_TASK_TIME)
        acsq_data_set = {i[1] for i in acsq_data}
        acsq_data_dict = {value: key for key, value in enumerate(list(acsq_data_set))}
        pid = cls.TRACE_PID_MAP.get(TraceViewHeaderConstant.PEOCESS_ACSQ)
        meta_data = [["process_name", pid, InfoConfReader().get_json_tid_data(), TraceViewHeaderConstant.PEOCESS_ACSQ]]
        for tid_value_index, tid_value in enumerate(list(acsq_data_set)):
            meta_data.append(["thread_name", pid, tid_value_index, "Stream {}".format(tid_value)])
        result = TraceViewManager.metadata_event(meta_data)
        try:
            if acsq_data:
                task_trace_datas = cls._add_acsq_opname(acsq_data, result_dir, acsq_data_dict)
                result.extend(TraceViewManager.time_graph_trace(
                    TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, task_trace_datas))
        except (TypeError, IndexError, ValueError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        return result

    @classmethod
    def _add_acsq_opname(cls: any, data_list: list, result_dir: str, tid_dict: dict):
        task_trace_datas = []
        op_names = cls._get_acsq_opname(PathManager.get_sql_dir(result_dir))
        for data in data_list:
            if data[2] <= 0 or data[3] < 0:
                continue
            op_name = cls._get_task_trace_value('{}_{}'.format(str(data[1]), str(data[0])), op_names)
            trace_data_args = OrderedDict(
                [
                    ("Task Type", data[4]),
                    ("OP Name", op_name),
                    ("Stream Id", data[1]),
                    ("Task Id", data[0]),
                ])
            task_type = str(data[4])
            if Constant.NA != op_name:
                task_type += '@' + op_name
            task_trace_datas.append([task_type,
                                     cls.TRACE_PID_MAP.get(TraceViewHeaderConstant.PEOCESS_ACSQ),
                                     tid_dict.get(data[1]),
                                     float(data[2]) / DBManager.NSTOUS,
                                     int(data[3]) / DBManager.NSTOUS if data[3] > 0 else 0,
                                     trace_data_args])
        return task_trace_datas

    @classmethod
    def _get_acsq_opname(cls: any, sql_path: str) -> dict:
        op_names = {}
        sql = "SELECT op_name, stream_id, task_id FROM {} ".format(DBNameConstant.TABLE_SUMMARY_GE)
        conn_ge, cur_ge = DBManager.check_connect_db_path(
            os.path.join(sql_path, DBNameConstant.DB_AICORE_OP_SUMMARY))
        try:
            if conn_ge and cur_ge:
                if not DBManager.judge_table_exist(cur_ge, DBNameConstant.TABLE_SUMMARY_GE):
                    return op_names
                ge_datas = DBManager.fetch_all_data(cur_ge, sql)
                if not ge_datas:
                    return op_names
                for ge_data in ge_datas:
                    ge_data_key = "_".join(list(map(str, [ge_data[1], ge_data[2]])))
                    op_names[ge_data_key] = ge_data[0]
            return op_names
        except ValueError as err:
            logging.error(str(err))
            return op_names
        finally:
            DBManager.destroy_db_connect(conn_ge, cur_ge)

    @classmethod
    def _get_task_scheduler_data(cls: any, sql_path: str) -> list:
        result_data = []
        db_path = os.path.join(sql_path, DBNameConstant.DB_AICORE_OP_SUMMARY)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not conn or not curs or not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_SUMMARY_TASK_TIME):
            return result_data
        sql = "SELECT stream_id, task_id, start_time, duration_time, task_type, index_id, batch_id " \
              "FROM {table_name} WHERE start_time>0 and duration_time>=0" \
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
        rts_task_type = cls.get_rts_track_task_type(sql_path)
        total_cycle, total_time = cls.get_total_cycle(sql_path)
        for sql_data in sql_datas:
            # key: stream_id task_id task_type batch_id
            _key_for_ops = "_".join([str(sql_data[0]), str(sql_data[1]), str(sql_data[-1])])
            total_time_value = cls._get_task_trace_value(_key_for_ops, total_time)
            if is_number(total_time_value):
                total_time_value = round(total_time_value, CommonConstant.ROUND_SIX)
            trace_data_args = OrderedDict(
                [("Task Type", cls._get_task_trace_value(_key_for_ops, task_types,
                                                         cls._get_task_trace_value(_key_for_ops, rts_task_type,
                                                                                   Constant.TASK_TYPE_OTHER))),
                 ("Stream Id", sql_data[0]),
                 ("Task Id", sql_data[1]),
                 ("Aicore Time(ms)", total_time_value),
                 ("Total Cycle", cls._get_task_trace_value(_key_for_ops, total_cycle))
                 ])

            trace_data_pice = [
                cls._get_task_trace_value(
                    _key_for_ops, op_names, cls._get_task_trace_value(
                        _key_for_ops, rts_task_type, Constant.TASK_TYPE_OTHER)),
                NumberConstant.TASK_TIME_PID,
                sql_data[0],
                float(sql_data[2]) / DBManager.NSTOUS,
                int(sql_data[3]) / DBManager.NSTOUS if sql_data[3] > 0 else 0,
                trace_data_args
            ]

            trace_data.append(trace_data_pice)
        return trace_data

    @classmethod
    def _get_task_trace_value(cls: any, _key_for_ops: str, data: dict, default_value: str = Constant.NA) -> any:
        return data.get(_key_for_ops, default_value)

    @classmethod
    def _get_ai_cpu_data(cls: any, job_path: str = None) -> list:
        ai_cpu_datas = []
        dir_path = job_path
        ai_cpu_data = ParseAiCpuData.get_ai_cpu_from_ts(dir_path)
        op_names, _ = cls.get_op_names_and_task_type(PathManager.get_sql_dir(dir_path))
        result_data = []
        if ai_cpu_data:
            result_data.extend(cls.get_meta_trace_data(TraceViewHeaderConstant.PROCESS_AI_CPU))
            for stream_id, task_id, sys_start, sys_end, batch_id in ai_cpu_data:
                _key_for_ops = "_".join([str(stream_id), str(task_id), str(batch_id)])
                duration = (float(sys_end) - float(sys_start)) * NumberConstant.MS_TO_US
                ai_cpu_datas.append(
                    (cls._get_task_trace_value(_key_for_ops, op_names),
                     cls.TRACE_PID_MAP.get(TraceViewHeaderConstant.PROCESS_AI_CPU, 0),
                     InfoConfReader().get_json_tid_data(),
                     float(sys_start) * NumberConstant.MS_TO_US,
                     duration,
                     OrderedDict([("Task Type", "AI_CPU"),
                                  ("Task Time(us)", duration)])))
            result_data.extend(
                TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, ai_cpu_datas))
        return result_data

    @classmethod
    def _get_all_reduce_data(cls: any, device_id: int, iter_range: IterationRange, result_dir: str) -> list:
        sql_path = PathManager.get_sql_dir(result_dir)
        result_data = []
        trace_conn, trace_curs = DBManager.check_connect_db_path(
            os.path.join(sql_path, DBNameConstant.DB_TRACE))
        if not trace_conn or not trace_curs:
            return result_data
        if not DBManager.judge_table_exist(trace_curs, DBNameConstant.TABLE_TRAINING_TRACE) or \
                not DBManager.judge_table_exist(trace_curs, DBNameConstant.TABLE_ALL_REDUCE):
            return result_data

        result_data = cls.get_meta_trace_data(TraceViewHeaderConstant.PROCESS_ALL_REDUCE)
        all_reduce_datas = \
            cls._format_all_reduce_data(trace_curs, device_id, iter_range)
        result_data.extend(
            TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, all_reduce_datas))
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        return result_data

    @classmethod
    def _format_all_reduce_data(cls: any, trace_curs: any, device_id: int, iter_range: IterationRange) -> list:
        """
        get all reduce trace data
        :param trace_curs: curs related to trace_db
        :param device_id: device id
        :param iter_range: iter range
        :return: all_reduce_data
        """
        all_reduce_datas = []
        hwts_freq = InfoConfReader().get_freq(StrConstant.HWTS)

        for training_trace_data in cls._get_trace_one_device(trace_curs, device_id, iter_range):
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
