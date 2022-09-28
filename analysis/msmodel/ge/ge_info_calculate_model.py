#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

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

    def get_all_ge_static_shape_data(self: any, datatype: str) -> list:
        all_ge_static_shape_data = [{}, {}]
        if not Utils.is_step_scene(self.result_dir):
            return all_ge_static_shape_data
        ge_static_shape_data = self._get_ge_static_shape_data(datatype)
        if not ge_static_shape_data:
            return all_ge_static_shape_data
        step_trace_primary_key_data = self._get_step_trace_primary_key_data()
        iter_model_dict = {}
        for primary_key in step_trace_primary_key_data:
            if primary_key[0] in ge_static_shape_data.keys():
                iter_model_dict[primary_key[2]] = primary_key[0]
        all_ge_static_shape_data[0] = iter_model_dict
        all_ge_static_shape_data[1] = ge_static_shape_data
        return all_ge_static_shape_data

    def get_all_ge_non_static_shape_data(self: any, datatype: str) -> dict:
        if not Utils.is_step_scene(self.result_dir):
            return {}
        non_static_shape_data = self._get_ge_no_static_shape_data(datatype)
        if not non_static_shape_data:
            return {}
        step_trace_primary_key_data = self._get_step_trace_primary_key_data()
        for primary_key in step_trace_primary_key_data:
            model_index = "{0}-{1}".format(primary_key[0], primary_key[1])
            if model_index in non_static_shape_data.keys():
                non_static_shape_data[primary_key[2]] = non_static_shape_data.pop(model_index)
        return non_static_shape_data

    def _get_step_trace_primary_key_data(self: any) -> list:
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.create_connect_db(db_path)
        sql = "select model_id, index_id, iter_id from {0}".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        step_trace_primary_key_data = DBManager.fetch_all_data(trace_curs, sql)
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        return step_trace_primary_key_data

    def _get_ge_static_shape_data(self: any, datatype: str) -> dict:
        static_shape_data_sql = "select model_id, GROUP_CONCAT(stream_id||'-'||task_id||'-'||batch_id) from {0} " \
                                "where index_id=0 and task_type='{1}' " \
                                "group by model_id".format(DBNameConstant.TABLE_GE_TASK, datatype)
        static_shape_data = DBManager.fetch_all_data(self.cur, static_shape_data_sql)
        static_shape_dict = {}
        if static_shape_data:
            for static_shape in static_shape_data:
                static_shape_dict[static_shape[0]] = set(static_shape[1].split(','))
        return static_shape_dict

    def _get_ge_no_static_shape_data(self: any, datatype: str) -> list:
        non_static_shape_data_sql = "select model_id||'-'||index_id, " \
                                    "GROUP_CONCAT(stream_id||'-'||task_id||'-'||batch_id) from {0} " \
                                    "where index_id<>0 and task_type='{1}' " \
                                    "group by model_id||'-'||index_id".format(DBNameConstant.TABLE_GE_TASK, datatype)
        non_static_shape_data = DBManager.fetch_all_data(self.cur, non_static_shape_data_sql)
        non_static_shape_dict = {}
        if non_static_shape_data:
            for non_static_shape in non_static_shape_data:
                non_static_shape_dict[non_static_shape[0]] = set(non_static_shape[1].split(','))
        return non_static_shape_dict
