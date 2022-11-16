#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.

import logging
import os

from common_func.common import CommonConstant
from common_func.common import check_number_valid
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant


class TrainingTraceSaver:
    """
    saver of training trace data
    """

    @staticmethod
    def save_data_to_db(message: dict) -> bool:
        """
        save message to db
        """
        if not message:
            logging.error("reduce_cp message empty or data empty")
            return False
        result = TrainingTraceSaver._check_training_trace(message)
        if not result:
            return False
        host_id = message["host_id"]
        device_id = message["device_id"]
        conn, curs = TrainingTraceSaver.create_trace_conn(message["sql_path"])
        TrainingTraceSaver.__insert_values(conn, host_id, device_id, message["data"])
        TrainingTraceSaver.__update_iteration_id(conn)
        TrainingTraceSaver.__update_data_aug_bound(conn, device_id)
        DBManager.destroy_db_connect(conn, curs)
        return True

    @staticmethod
    def save_file_relation_to_db(message: dict) -> bool:
        """
        Rewrite gRPC send_file_relation method.
        Send data file path of a job_id to store in db.
        """
        if not message:
            logging.error("send_file_relation message empty")
            return False
        conn, curs = TrainingTraceSaver.create_trace_conn(message["sql_path"])
        insert_sql = 'insert or ignore into {0} values (?)'.format(DBNameConstant.TABLE_TRACE_FILES)
        result = DBManager.execute_sql(conn, insert_sql, (message["file_path"],))
        DBManager.destroy_db_connect(conn, curs)
        return result

    @staticmethod
    def create_trace_conn(sql_path: str) -> any:
        """
        create connection to DB
        """
        conn_path = os.path.join(sql_path, DBNameConstant.DB_TRACE)
        if os.path.exists(conn_path):
            return DBManager.create_connect_db(conn_path)
        conn, curs = DBManager.create_connect_db(conn_path)
        os.chmod(conn_path, CommonConstant.CONN_MASK)
        DBManager.execute_sql(conn, "pragma page_size=4096")
        DBManager.execute_sql(conn, "pragma cache_size={0}".format(256 * 1024))
        cp_sql = "create table if not exists {0}(host_id int, device_id int, iteration_id int, " \
                 "job_stream int, job_task int, FP_start int, FP_stream int, FP_task int, " \
                 "BP_end int, BP_stream int, BP_task int, iteration_end int, iter_stream int," \
                 "iter_task int, iteration_time int, fp_bp_time int, grad_refresh_bound int, " \
                 "data_aug_bound int default 0, model_id int default {1}, " \
                 "primary key(device_id,iteration_end))".format(
            DBNameConstant.TABLE_TRAINING_TRACE, NumberConstant.DEFAULT_MODEL_ID)

        DBManager.execute_sql(conn, cp_sql)
        for field in ("iteration_time", "fp_bp_time", "grad_refresh_bound", "data_aug_bound"):
            index_sql = "create index if " \
                        "not exists {0} on {1} ({0} desc)".format(field, DBNameConstant.TABLE_TRAINING_TRACE)
            DBManager.execute_sql(conn, index_sql)
        all_reduce_sql = "create table if not exists {0}(host_id int, device_id int," \
                         "iteration_end int, start int, start_stream int, start_task int," \
                         "end int, end_stream int, end_task int, model_id int default {1}," \
                         "primary key(device_id," \
                         "iteration_end, start))".format(
            DBNameConstant.TABLE_ALL_REDUCE, NumberConstant.DEFAULT_MODEL_ID)
        DBManager.execute_sql(conn, all_reduce_sql)
        file_sql = "create table if not exists {}(file_path text, primary key(file_path))".format(
            DBNameConstant.TABLE_TRACE_FILES)
        DBManager.execute_sql(conn, file_sql)
        return conn, curs

    @staticmethod
    def _check_all_reduce(items: dict) -> bool:
        for item in items["all_reduces"]:
            check_int_item = [
                item["start"], item["start_stream"], item["start_task"],
                item["end"], item["end_stream"], item["end_task"]
            ]
            for check_item in check_int_item:
                if not check_number_valid(check_item):
                    logging.error("reduce_cp message check integer value failed")
                    return False
        return True

    @staticmethod
    def _check_training_trace(message: dict) -> bool:
        for items in message["data"]:
            check_int_item = [
                items["iteration_id"], items["job_stream"], items["job_task"],
                items["FP_start"], items["FP_stream"], items["FP_task"],
                items["BP_end"], items["BP_stream"], items["BP_task"],
                items["iteration_end"], items["iter_stream"], items["iter_task"]
            ]
            for item in check_int_item:
                if not check_number_valid(item):
                    logging.error("data message check integer value failed")
                    return False
            ret = TrainingTraceSaver._check_all_reduce(items)
            if not ret:
                return ret
        return True

    @staticmethod
    def __insert_values(conn: any, host_id: int, device_id: int, data: list) -> None:
        """
        Insert traing_trace protobuf messages into traing_trace table
        """
        rows = []
        all_reduces = []
        for i, trainingtrace in enumerate(data):
            data_aug_bound = trainingtrace["FP_start"] - data[i - 1]["iteration_end"] if i > 0 else 0
            iteration_time = trainingtrace["iteration_end"] - trainingtrace["FP_start"]
            if i > 0:
                iteration_time = trainingtrace["iteration_end"] - data[i - 1]["iteration_end"]
            rows.append((host_id,
                         device_id,
                         trainingtrace["iteration_id"],
                         trainingtrace["job_stream"], trainingtrace["job_task"],
                         trainingtrace["FP_start"], trainingtrace["FP_stream"],
                         trainingtrace["FP_task"], trainingtrace["BP_end"],
                         trainingtrace["BP_stream"], trainingtrace["BP_task"],
                         trainingtrace["iteration_end"],
                         trainingtrace["iter_stream"], trainingtrace["iter_task"],
                         iteration_time,
                         trainingtrace["BP_end"] - trainingtrace["FP_start"],
                         trainingtrace["iteration_end"] - trainingtrace["BP_end"],
                         data_aug_bound,
                         NumberConstant.DEFAULT_MODEL_ID))
            for all_reduce in trainingtrace["all_reduces"]:
                all_reduces.append((host_id,
                                    device_id,
                                    trainingtrace["iteration_end"],
                                    all_reduce["start"], all_reduce["start_stream"],
                                    all_reduce["start_task"], all_reduce["end"],
                                    all_reduce["end_stream"], all_reduce["end_task"],
                                    NumberConstant.DEFAULT_MODEL_ID))
        insert_sql = 'insert into {0} values ({1})' \
            .format(DBNameConstant.TABLE_TRAINING_TRACE, "?," * (len(rows[0]) - 1) + "?")
        DBManager.executemany_sql(conn, insert_sql, rows)
        if all_reduces:
            insert_reduce = 'insert into {0} values ({1})' \
                .format(DBNameConstant.TABLE_ALL_REDUCE, "?," * (len(all_reduces[0]) - 1) + "?")
            DBManager.executemany_sql(conn, insert_reduce, all_reduces)

    @staticmethod
    def __update_iteration_id(conn: any) -> None:
        """
        update all iteration_id after insert nwe data.
        """
        update_sql = 'update {0} set iteration_id=(select count(1) from {0} b ' \
                     'where {0}.device_id=b.device_id ' \
                     'and {0}.iteration_end >= b.iteration_end)'.format(DBNameConstant.TABLE_TRAINING_TRACE)
        DBManager.execute_sql(conn, update_sql)

    @staticmethod
    def __update_data_aug_bound(conn: any, device_id: int) -> None:
        """
        Update last traing_trace table with the latest cp protobuf message.
        """
        update_sql = "update {0} set data_aug_bound= FP_start - (select iteration_end from {0} a " \
                     "where {0}.device_id=a.device_id " \
                     "and a.iteration_id = {0}.iteration_id-1)" \
                     "where device_id=? and data_aug_bound = 0 " \
                     "and iteration_id > 1".format(DBNameConstant.TABLE_TRAINING_TRACE)
        DBManager.execute_sql(conn, update_sql, (device_id,))
