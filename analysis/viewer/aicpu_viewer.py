#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import logging
import os
from collections import deque
from dataclasses import dataclass
from typing import Union
from typing import List

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_iteration import MsprofIteration
from common_func.path_manager import PathManager
from common_func.profiling_scene import ProfilingScene
from msmodel.ai_cpu.ai_cpu_model import AiCpuModel
from profiling_bean.db_dto.ge_task_dto import GeTaskDto
from profiling_bean.db_dto.step_trace_dto import IterationRange


@dataclass
class AiCpuData:
    stream_id: int = None
    task_id: int = None
    sys_start: int = None
    sys_end: int = None
    node_name: str = None
    compute_time: float = None
    memcpy_time: float = None
    task_time: float = None
    dispatch_time: float = None
    total_time: float = None
    batch_id: int = None


class ParseAiCpuData:
    """
    class for parse aicpu data
    """

    @staticmethod
    def analysis_aicpu(project_path: str, iter_range: IterationRange) -> tuple:
        """
        parse and analysis AI CPU related dp data
        :return: ai cpu data , headers
        """
        headers = [
            "Timestamp(us)", "Node", "Compute_time(us)", "Memcpy_time(us)", "Task_time(us)",
            "Dispatch_time(us)", "Total_time(us)", "Stream ID", "Task ID"
        ]
        ai_cpu_results = ParseAiCpuData.get_ai_cpu_data(project_path, iter_range)
        ascend_task_results = ParseAiCpuData.get_ascend_task_ai_cpu_data(project_path)
        ge_results = ParseAiCpuData.get_ge_summary_aicpu_data(project_path)

        res = ParseAiCpuData.get_aicpu_batch_id(ai_cpu_results, ascend_task_results)
        res = ParseAiCpuData.match_aicpu_with_ge_summary(res, ge_results)
        res.sort(key=lambda x: x[0])
        return headers, res

    @staticmethod
    def get_aicpu_batch_id(ai_cpu_data: List[AiCpuData], ascend_task_data: List[GeTaskDto]) -> List[AiCpuData]:
        """
        get ai cpu batch_id from ascend_task data
        """
        sep_ai_cpu_dict = ParseAiCpuData._sep_task_by_stream_task(ai_cpu_data)
        seq_ascend_task_dict = ParseAiCpuData._sep_task_by_stream_task(ascend_task_data)

        res = []
        # ascend_task's tasks num more than aicpu's tasks num,
        # and csv's data displayed mainly by aicpu data
        stream_task_set = sep_ai_cpu_dict.keys()
        for key in stream_task_set:
            ai_cpu_t = sep_ai_cpu_dict.get(key, [])
            ascend_task_t = seq_ascend_task_dict.get(key, [])
            match_res = ParseAiCpuData._get_aicpu_batch_id(ai_cpu_t, ascend_task_t)
            if not match_res:
                logging.error("Can't match aicpu data batch_id, it's stream_id: %d, task_id: %d",
                              key[0], key[1])
                continue
            res.extend([*match_res])
        return res

    @staticmethod
    def match_aicpu_with_ge_summary(ai_cpu_data: List[AiCpuData], ge_summary_data: List[GeTaskDto]) -> list:
        """
        get ai cpu op_name from ge_summary data
        """
        ge_summary_dic = {}
        for dto in ge_summary_data:
            if (dto.stream_id, dto.task_id, dto.batch_id) in ge_summary_dic:
                logging.error("There are aicpu data with the same stream_id: %d, task_id: %d, "
                              "batch_id: %d", dto.stream_id, dto.task_id, dto.batch_id)
                continue
            ge_summary_dic.setdefault((dto.stream_id, dto.task_id, dto.batch_id), dto.op_name)
        res = [0, ] * len(ai_cpu_data)
        for i, dto in enumerate(ai_cpu_data):
            if (dto.stream_id, dto.task_id, dto.batch_id) in ge_summary_dic:
                dto.node_name = ge_summary_dic[(dto.stream_id, dto.task_id, dto.batch_id)]
            res[i] = [
                dto.sys_start, dto.node_name, dto.compute_time, dto.memcpy_time,
                dto.task_time, dto.dispatch_time, dto.total_time, dto.stream_id, dto.task_id
            ]
        return res

    @staticmethod
    def get_ge_summary_aicpu_data(project_path: str) -> list:
        """
        get ge_summary data
        """
        db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_AICORE_OP_SUMMARY)
        ge_summary_conn, ge_summary_curs = DBManager.check_connect_db_path(db_path)
        if not ge_summary_conn or not ge_summary_curs:
            logging.warning("Can't connect ai_core_op_summary.db!")
            return []

        ge_results = ParseAiCpuData._get_ge_summary_aicpu_data(ge_summary_conn)
        DBManager.destroy_db_connect(ge_summary_conn, ge_summary_curs)
        return ge_results

    @staticmethod
    def get_ascend_task_ai_cpu_data(project_path: str) -> list:
        """
        get ascend_task_data
        """
        db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_ASCEND_TASK)
        ascend_task_conn, ascend_task_curs = DBManager.check_connect_db_path(db_path)
        if not ascend_task_conn or not ascend_task_curs:
            logging.warning("Can't connect ascend_task.db!")
            return []

        ascend_task_results = ParseAiCpuData._get_ascend_task_aicpu_data(ascend_task_conn)
        DBManager.destroy_db_connect(ascend_task_conn, ascend_task_curs)
        return ascend_task_results

    @staticmethod
    def get_ai_cpu_data(project_path: str, iter_range: IterationRange) -> list:
        """
        get ai cpu data
        """
        db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_AI_CPU)
        ai_cpu_conn, ai_cpu_curs = DBManager.check_connect_db_path(db_path)
        if not ai_cpu_conn or not ai_cpu_curs:
            logging.warning("Can't connect ai_cpu.db!")
            return []

        ai_cpu_results = ParseAiCpuData._get_aicpu_data(ai_cpu_conn, iter_range, project_path)
        DBManager.destroy_db_connect(ai_cpu_conn, ai_cpu_curs)
        return ai_cpu_results

    @staticmethod
    def get_ai_cpu_from_ts(project_path: str) -> list:
        """
        get ai cpu query sql
        """
        aicpu_model = AiCpuModel(project_path)
        if not os.path.exists(PathManager.get_db_path(project_path, DBNameConstant.DB_AI_CPU)):
            logging.info("no aicpu db found")
            return []
        with aicpu_model:
            aicpu_data = aicpu_model.get_ai_cpu_data_from_ts()
        return aicpu_data

    @staticmethod
    def _sep_task_by_stream_task(tasks: Union[List[GeTaskDto], List[AiCpuData]]) -> dict:
        ret = {}
        for task in tasks:
            ret.setdefault((task.stream_id, task.task_id), []).append(task)
        return ret

    @staticmethod
    def _get_aicpu_batch_id(ai_cpu_data: List[AiCpuData], ascend_task_data: List[GeTaskDto]) -> List:
        if len(ai_cpu_data) != len(ascend_task_data):
            return []
        ai_cpu_queue = deque(ai_cpu_data)
        ascend_task_queue = deque(ascend_task_data)
        for ai_cpu in ai_cpu_queue:
            ai_cpu.batch_id = ascend_task_queue.popleft().batch_id
        return list(ai_cpu_queue)

    @staticmethod
    def _get_ge_summary_aicpu_data(ge_summary_conn):
        sql = "select op_name, stream_id, task_id, batch_id from {0} where task_type = '{1}' " \
              "order by timestamp".format(DBNameConstant.TABLE_SUMMARY_GE,
                                          Constant.TASK_TYPE_AI_CPU)
        return DBManager.fetch_all_data(ge_summary_conn.cursor(), sql, dto_class=GeTaskDto)

    @staticmethod
    def _get_ascend_task_aicpu_data(ascend_task_conn):
        sql = "select batch_id, stream_id, task_id from {0} where host_task_type = 'KERNEL_AICPU' " \
              "order by start_time".format(DBNameConstant.TABLE_ASCEND_TASK)
        return DBManager.fetch_all_data(ascend_task_conn.cursor(), sql, dto_class=GeTaskDto)

    @staticmethod
    def _get_aicpu_data(ai_cpu_conn, iter_range, project_path):
        where_condition = ""
        if not ProfilingScene().is_all_export():
            iter_time = MsprofIteration(project_path).get_iter_interval(iter_range)
            if iter_time:
                where_condition = "where sys_start>={0} " \
                                  "and sys_end<={1}".format(iter_time[0] / NumberConstant.MS_TO_NS,
                                                            iter_time[1] / NumberConstant.MS_TO_NS)
        sql = "select sys_start*{MS_TO_US}+{local_time_offset} as sys_start,'{node_name}' as node_name," \
              "compute_time*{MS_TO_US} as compute_time, memcpy_time*{MS_TO_US} as memcpy_time," \
              "task_time*{MS_TO_US} as task_time,dispatch_time*{MS_TO_US} as dispatch_time," \
              "total_time*{MS_TO_US} as total_time, stream_id, task_id from {0} {where_condition} " \
              "order by sys_start".format(DBNameConstant.TABLE_AI_CPU,
                                          MS_TO_US=NumberConstant.MS_TO_US,
                                          local_time_offset=InfoConfReader().get_local_time_offset(),
                                          node_name=Constant.NA,
                                          where_condition=where_condition)
        return DBManager.fetch_all_data(ai_cpu_conn.cursor(), sql, dto_class=AiCpuData)
