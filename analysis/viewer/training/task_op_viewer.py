#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import logging
import sqlite3

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_manager import ClassRowType
from common_func.db_name_constant import DBNameConstant
from profiling_bean.db_dto.ge_task_dto import GeTaskDto
from viewer.memory_copy.memory_copy_viewer import MemoryCopyViewer


class TaskOpViewer:
    """
    viewer of training trace data
    """

    @staticmethod
    def get_task_op_summary(message: dict) -> tuple:
        """
        @param message
        Rewrite gRPC task op method.
        """
        headers = ["kernel_name", "kernel_type", "stream_id", "task_id",
                   "task_time(us)", "task_start(ns)", "task_stop(ns)"]
        if not message:
            logging.error("get_task_op_summary message empty")
            return headers, [], 0
        hwts_conn, hwts_curs = DBManager.check_connect_db(message.get("result_dir"), DBNameConstant.DB_HWTS)
        task_conn, task_curs = DBManager.check_connect_db(message.get("result_dir"), DBNameConstant.DB_RTS_TRACK)
        ge_conn, ge_curs = DBManager.check_connect_db(message.get("result_dir"), DBNameConstant.DB_GE_INFO)
        if ProfilingScene().is_operator():
            data, _ = TaskOpViewer.get_op_task_data_summary(hwts_curs, ge_curs)
        else:
            data, _ = TaskOpViewer.get_task_data_summary(hwts_curs, task_curs, ge_curs)
        DBManager.destroy_db_connect(hwts_conn, hwts_curs)
        DBManager.destroy_db_connect(task_conn, task_curs)
        DBManager.destroy_db_connect(ge_conn, ge_curs)
        if not data:
            return headers, [], 0
        data = TaskOpViewer._add_memcpy_data(message.get("result_dir"), data)
        return headers, data, len(data)

    @staticmethod
    def get_task_data_summary(hwts_curs: any, task_curs: any, ge_curs: any) -> tuple:
        """
        get task info csv
        """
        if not DBManager.judge_table_exist(ge_curs, DBNameConstant.TABLE_GE_TASK):
            logging.warning(
                "No ge data collected, maybe the TaskInfo table is not created, try to export data with no ge data")
        if not DBManager.judge_table_exist(task_curs, DBNameConstant.TABLE_RUNTIME_TRACK):
            logging.warning("No need to export task time data, maybe the RuntimeTrack table is not created.")
            return [], 0
        if not DBManager.judge_table_exist(hwts_curs, DBNameConstant.TABLE_HWTS_TASK_TIME):
            logging.warning("No need to export task time data, maybe the hwts is not collected.")
            return [], 0

        sql = "SELECT stream_id, task_id, running," \
              "complete, batch_id from {0}".format(DBNameConstant.TABLE_HWTS_TASK_TIME)
        hwts_data = DBManager.fetch_all_data(hwts_curs, sql)

        try:
            task_info = TaskOpViewer._reformat_task_info(hwts_data, task_curs, ge_curs)
        except sqlite3.Error as error:
            logging.error(str(error), exc_info=Constant.TRACE_BACK_SWITCH)
            return [], 0
        return task_info, len(task_info)

    @staticmethod
    def get_op_task_data_summary(hwts_curs: any, ge_curs: any) -> tuple:
        """
        get task time data for operator scene
        :param hwts_curs: cursor of hwts
        :param ge_curs: cursor of ge
        :return: task data and count of the task data
        """

        if not DBManager.judge_table_exist(ge_curs, DBNameConstant.TABLE_GE_TASK):
            logging.warning(
                "No ge data collected, maybe the TaskInfo table is not created, try to export data with no ge data")
        if not DBManager.judge_table_exist(hwts_curs, DBNameConstant.TABLE_HWTS_TASK_TIME):
            logging.warning("No need to export task time data, maybe the hwts is not collected.")
            return [], 0

        sql = "SELECT stream_id, task_id, running, " \
              "complete, batch_id from {0}".format(DBNameConstant.TABLE_HWTS_TASK_TIME)
        hwts_data = DBManager.fetch_all_data(hwts_curs, sql)

        sql = "SELECT stream_id, task_id, op_name, " \
              "task_type, batch_id from {0}".format(DBNameConstant.TABLE_GE_TASK)
        ge_data = DBManager.fetch_all_data(ge_curs, sql)

        try:
            task_info = TaskOpViewer._reformat_op_task_info(hwts_data, ge_data)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as error:
            logging.error(str(error), exc_info=Constant.TRACE_BACK_SWITCH)
            return [], 0
        return task_info, len(task_info)

    @staticmethod
    def _add_memcpy_data(result_dir: str, data: list) -> list:
        memcpy_viewer = MemoryCopyViewer(result_dir)
        memcpy_data = memcpy_viewer.get_memory_copy_non_chip0_summary()
        data.extend(memcpy_data)

        return data

    @staticmethod
    def _reformat_task_info(task_data: list, task_curs: any, ge_curs: any) -> list:
        ge_sql = "SELECT op_name, stream_id, task_id, batch_id from {0}".format(
            DBNameConstant.TABLE_GE_TASK)
        track_sql = "SELECT tasktype, stream_id, task_id, batch_id from {0}".format(
            DBNameConstant.TABLE_RUNTIME_TRACK)

        ge_curs.row_factory = ClassRowType.class_row(GeTaskDto)
        task_curs.row_factory = ClassRowType.class_row(GeTaskDto)
        op_name_list = DBManager.fetch_all_data(ge_curs, ge_sql)
        track_data_list = DBManager.fetch_all_data(task_curs, track_sql)

        op_name_dict, task_type_dict = {}, {}
        if op_name_list:
            op_name_dict = {(row.stream_id, row.task_id, row.batch_id): row.op_name for row in op_name_list}
        if track_data_list:
            task_type_dict = {(row.stream_id, row.task_id, row.batch_id): row.tasktype for row in track_data_list}

        task_info = []
        for data in task_data:
            op_name = op_name_dict.get((data[0], data[1], data[4]), "N/A")
            task_type = task_type_dict.get((data[0], data[1], data[4]))
            if task_type and len(data) >= 4:  # 4 is the minimum length for hwts data array
                # task_data[0] is kernel name, task_data[1] is task type
                task_time = "N/A"
                if data[3] != 0:
                    task_time = str((data[3] - data[2]) / DBManager.NSTOUS)
                task_info.append((op_name, str(task_type), data[0],
                                  data[1], task_time,
                                  "\"" + str(data[2]) + "\"",
                                  "\"" + str(data[3]) + "\""))
        return task_info

    @staticmethod
    def _reformat_op_task_info(task_data: list, ge_data: list) -> list:
        task_info = []
        ge_data_dict = {}
        if ge_data:
            ge_data_dict = {"{}-{}-{}".format(ge_datum[0], ge_datum[1],
                                              ge_datum[4]): (ge_datum[2], ge_datum[3])
                            for ge_datum in ge_data}
        for task_datum in task_data:
            duration = str((task_datum[3] - task_datum[2]) / DBManager.NSTOUS)
            task_start = "".join(["\"", str(task_datum[2]), "\""])
            task_end = "".join(["\"", str(task_datum[3]), "\""])
            name_type = ge_data_dict.get("{}-{}-{}".format(task_datum[0], task_datum[1],
                                                           task_datum[4]), (Constant.NA, Constant.NA))
            if name_type == (Constant.NA, Constant.NA):
                logging.warning(
                    "Can not find name and type for stream %d, "
                    "task %d", task_datum[0], task_datum[1])
            task_info.append((name_type[0],  # op name
                              name_type[1],  # task type
                              task_datum[0],  # stream id
                              task_datum[1],  # task id
                              duration,  # duration
                              task_start,  # task start
                              task_end,  # task end
                              ))
        return task_info
