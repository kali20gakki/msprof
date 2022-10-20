#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_iteration import MsprofIteration
from common_func.path_manager import PathManager
from msmodel.ai_cpu.ai_cpu_model import AiCpuModel


class ParseAiCpuData:
    """
    class for parse aicpu data
    """

    @staticmethod
    def analysis_aicpu(project_path: str, iteration_id: int, model_id: int) -> tuple:
        """
        parse and analysis AI CPU related dp data
        :return: ai cpu data , headers
        """
        headers = ["Timestamp", "Node", "Compute_time(ms)", "Memcpy_time(ms)", "Task_time(ms)",
                   "Dispatch_time(ms)", "Total_time(ms)", "Stream ID", "Task ID"]
        ai_cpu_results = []
        db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_AI_CPU)
        ai_cpu_conn, ai_cpu_curs = DBManager.check_connect_db_path(db_path)
        if not ai_cpu_conn or not ai_cpu_curs:
            return headers, ai_cpu_results

        ai_cpu_results = ParseAiCpuData.get_ai_cpu_data(project_path, model_id, iteration_id, ai_cpu_conn)
        DBManager.destroy_db_connect(ai_cpu_conn, ai_cpu_curs)
        return headers, ai_cpu_results

    @staticmethod
    def get_ai_cpu_data(project_path: str, model_id: int, iteration_id: int, ai_cpu_conn: any) -> list:
        """
        get ai cpu query sql
        :return: control statement
        """

        if not (DBManager.check_tables_in_db(PathManager.get_db_path(project_path, DBNameConstant.DB_GE_INFO),
                                             DBNameConstant.TABLE_GE_TASK)
                and DBManager.attach_to_db(ai_cpu_conn, project_path, DBNameConstant.DB_GE_INFO,
                                           DBNameConstant.TABLE_GE_TASK)):
            sql = "select sys_start,'{op_name}',compute_time,memcpy_time,task_time,dispatch_time," \
                  "total_time, stream_id, task_id from {0}".format(DBNameConstant.TABLE_AI_CPU, op_name=Constant.NA)
            if not ProfilingScene().is_operator():
                iter_time = MsprofIteration(project_path).get_iteration_time(iteration_id, model_id)
                if iter_time:
                    sql = "select sys_start,'{op_name}'," \
                          "compute_time,memcpy_time,task_time,dispatch_time," \
                          "total_time, stream_id, task_id from {0} where sys_start>={1} " \
                          "and sys_end<={2}".format(DBNameConstant.TABLE_AI_CPU,
                                                    iter_time[0][0] / NumberConstant.MS_TO_US,
                                                    iter_time[0][1] / NumberConstant.MS_TO_US, op_name=Constant.NA)
            return DBManager.fetch_all_data(ai_cpu_conn.cursor(), sql)
        if ProfilingScene().is_operator():
            sql = "select sys_start,op_name,compute_time,memcpy_time,task_time,dispatch_time," \
                  "total_time, {0}.stream_id, {0}.task_id from {0} join {1} on " \
                  "{0}.stream_id={1}.stream_id and {0}.task_id={1}.task_id and {0}.batch_id={1}.batch_id " \
                  "and {1}.task_type='{task_type}'" \
                .format(DBNameConstant.TABLE_AI_CPU, DBNameConstant.TABLE_GE_TASK,
                        task_type=Constant.TASK_TYPE_AI_CPU)
            return DBManager.fetch_all_data(ai_cpu_conn.cursor(), sql)

        iter_time = MsprofIteration(project_path).get_iteration_time(iteration_id, model_id)
        iter_dict = MsprofIteration(project_path).get_iter_dict_with_index_and_model(iteration_id, model_id)
        ai_cpu_data = []
        if not iter_time:
            return ai_cpu_data

        sql = "select sys_start,op_name,compute_time,memcpy_time,task_time,dispatch_time, total_time, " \
              "{0}.stream_id, {0}.task_id from {0} join {3} on sys_start>={1} and sys_end<={2} " \
              "and {0}.stream_id={3}.stream_id and {0}.task_id={3}.task_id and {0}.batch_id={3}.batch_id " \
              "and ({3}.index_id=? or {3}.index_id=0) and {3}.model_id=? " \
              "and {3}.task_type='{task_type}'" \
            .format(DBNameConstant.TABLE_AI_CPU,
                    iter_time[0][0] / NumberConstant.MS_TO_US,
                    iter_time[0][1] / NumberConstant.MS_TO_US,
                    DBNameConstant.TABLE_GE_TASK,
                    model_id=model_id, index_id=iteration_id, task_type=Constant.TASK_TYPE_AI_CPU)

        for iteration_id, model_id in iter_dict.values():
            ai_cpu_data.extend(DBManager.fetch_all_data(ai_cpu_conn.cursor(), sql, (iteration_id, model_id)))
        return ai_cpu_data

    @staticmethod
    def get_ai_cpu_from_ts(project_path: str) -> list:
        """
        get ai cpu query sql
        :return: control statement
        """
        aicpu_model = AiCpuModel(project_path)
        with aicpu_model:
            aicpu_data = aicpu_model.get_ai_cpu_data_from_ts()
        return aicpu_data
