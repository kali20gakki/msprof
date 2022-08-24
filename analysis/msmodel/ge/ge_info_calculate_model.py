#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to operate ge info db
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""
import os
import logging
import sqlite3

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from common_func.utils import Utils
from msmodel.interface.base_model import BaseModel


class GeInfoModel(BaseModel):
    """
    class used to operate ge info db
    """
    STREAM_TASK_KEY_FMT = "{0}-{1}"
    STREAM_TASK_BATCH_KEY_FMT = "{0}-{1}-{2}"

    def __init__(self: any, result_dir: str) -> None:
        super(GeInfoModel, self).__init__(result_dir, DBNameConstant.DB_GE_INFO, DBNameConstant.TABLE_GE_TASK)

    def check_table(self: any) -> bool:
        """
        check table of ge task
        :return:
        """
        if not self.conn or not self.cur \
                or not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_GE_TASK):
            logging.warning("No ge data starting with framework is found, "
                            "please check the result_dir directory: %s",
                            os.path.join(os.path.basename(self.result_dir), 'data'))
            return False
        return True

    def map_model_to_iter(self: any) -> dict:
        """
        map (model_id, index_id) to iter_id
        :return: model_to_iter_dict
        """
        model_to_iter_dict = {}
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.create_connect_db(db_path)
        sql = "select model_id, index_id, iter_id from {0}".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        map_data = DBManager.fetch_all_data(trace_curs, sql)
        DBManager.destroy_db_connect(trace_conn, trace_curs)

        if not map_data:
            return model_to_iter_dict
        for map_datum in map_data:
            model_to_iter_dict.setdefault((map_datum[0], map_datum[1]), map_datum[2])
        return model_to_iter_dict

    def get_ge_data(self: any, datatype: str) -> dict:
        """
        get ge data
        :return: iter dict
        """
        ge_op_iter_dict = {}

        if Utils.is_single_op_scene(self.result_dir):
            self.__get_ge_data_op_scene(ge_op_iter_dict, datatype)

        if Utils.is_step_scene(self.result_dir):
            self.__get_ge_data_step_scene(ge_op_iter_dict, datatype)

        return ge_op_iter_dict

    def get_batch_dict(self: any, datatype: str) -> dict:
        """
        get batch data
        :params: datatype
        :return: dict of iter id, stream id, task_id, batch id
        """
        ge_sql = "select model_id, index_id, stream_id, task_id, batch_id " \
                 "from {0} inner join( " \
                 "select min(timestamp) as timestamp " \
                 "from {0} where index_id != 0 and task_type = '{1}' " \
                 "group by model_id, index_id, stream_id) as min_time_table " \
                 "on {0}.timestamp = min_time_table.timestamp".format(
            DBNameConstant.TABLE_GE_TASK, datatype)
        ge_data = DBManager.fetch_all_data(self.cur, ge_sql)

        if Utils.is_single_op_scene(self.result_dir):
            return {(NumberConstant.INVALID_ITER_ID, stream_id): (task_id, batch_id)
                    for _, _, stream_id, task_id, batch_id in ge_data}
        else:
            model_to_iter_dict = self.map_model_to_iter()

            batch_dict = {}
            for model_id, index_id, stream_id, task_id, batch_id in ge_data:
                if (model_id, index_id) in model_to_iter_dict:
                    iter_id = model_to_iter_dict.get((model_id, index_id))
                    batch_dict.setdefault((iter_id, stream_id), (task_id, batch_id))
            return batch_dict

    def __update_iter_dict(self: any, model_to_iter_dict: dict, ge_op_iter_dict: dict, ge_data: list) -> None:
        for key, value in model_to_iter_dict.items():
            if key[0] == ge_data[0]:
                ge_op_iter_dict.setdefault(str(value), set()).add(
                    self.STREAM_TASK_KEY_FMT.format(ge_data[2], ge_data[3]))

    def __get_ge_data_step_scene(self: any, ge_op_iter_dict: dict, datatype: str) -> None:
        """
        get ge task data
        :return: iter dict
        """
        model_to_iter_dict = self.map_model_to_iter()
        ge_sql = "select model_id, index_id, stream_id, task_id, batch_id from {0} " \
                 "where (index_id=0 or index_id>0) and task_type='{1}'" \
            .format(DBNameConstant.TABLE_GE_TASK,
                    datatype)
        if DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_GE_TENSOR):
            ge_sql = "select {0}.model_id, {0}.index_id, {0}.stream_id, {0}.task_id, {0}.batch_id from {0} " \
                     "inner join {1} on {0}.task_id={1}.task_id and {0}.stream_id={1}.stream_id " \
                     "and {0}.timestamp={1}.timestamp " \
                     "and ({0}.index_id=0 or {0}.index_id>0) and {0}.task_type='{2}'" \
                .format(DBNameConstant.TABLE_GE_TASK,
                        DBNameConstant.TABLE_GE_TENSOR,
                        datatype)
        ge_dynamic_data = DBManager.fetch_all_data(self.cur, ge_sql)
        for per_data in ge_dynamic_data:
            if (per_data[0], per_data[1]) not in model_to_iter_dict:
                if per_data[1] == NumberConstant.STATIC_SHAPE_ITER_ID:
                    self.__update_iter_dict(model_to_iter_dict, ge_op_iter_dict, per_data)
                else:
                    logging.warning("the combination of ge model id and index id "
                                    "can't match with step trace data")
            else:
                iter_id = model_to_iter_dict.get((per_data[0], per_data[1]))
                ge_op_iter_dict.setdefault(str(iter_id), set()).add(
                    self.STREAM_TASK_BATCH_KEY_FMT.format(*per_data[2:]))

    def __get_ge_data_op_scene(self: any, ge_op_iter_dict: dict, datatype: str) -> None:
        """
        get ge task data
        :return: iter dict
        """
        ge_sql = "select model_id, index_id, stream_id, task_id, batch_id from {0} " \
                 "where task_type='{1}'" \
            .format(DBNameConstant.TABLE_GE_TASK,
                    datatype)
        if DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_GE_TENSOR):
            ge_sql = "select {0}.model_id, {0}.index_id, {0}.stream_id, {0}.task_id, {0}.batch_id from {0} " \
                     "inner join {1} on {0}.task_id={1}.task_id and {0}.stream_id={1}.stream_id " \
                     "and {0}.timestamp={1}.timestamp " \
                     "and {0}.task_type='{2}'" \
                .format(DBNameConstant.TABLE_GE_TASK,
                        DBNameConstant.TABLE_GE_TENSOR,
                        datatype)
        ge_op_data = DBManager.fetch_all_data(self.cur, ge_sql)
        op_set = set()
        for model_id, index, stream_id, task_id, batch_id in ge_op_data:
            if index == NumberConstant.STATIC_SHAPE_ITER_ID:
                op_set.add(self.STREAM_TASK_KEY_FMT.format(stream_id, task_id))
            else:
                op_set.add(self.STREAM_TASK_BATCH_KEY_FMT.format(stream_id, task_id, batch_id))
        ge_op_iter_dict.setdefault(str(NumberConstant.INVALID_ITER_ID), op_set)
