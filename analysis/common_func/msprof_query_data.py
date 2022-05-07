# coding=utf-8
"""
Function: Data splicing for command line of query interface.
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import os
import sqlite3

from common_func.common import error
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_common import MsProfCommonConstant
from common_func.path_manager import PathManager
from profiling_bean.basic_info.query_data_bean import QueryDataBean


class MsprofQueryData:
    """
    Data splicing for query data, and show in the screen later.
    """

    FILE_NAME = os.path.basename(__file__)
    QUERY_HEADERS = [MsProfCommonConstant.JOB_INFO, MsProfCommonConstant.DEVICE_ID, MsProfCommonConstant.JOB_NAME,
                     MsProfCommonConstant.COLLECTION_TIME, MsProfCommonConstant.MODEL_ID,
                     MsProfCommonConstant.ITERATION_ID, MsProfCommonConstant.TOP_TIME_ITERATION]
    QUERY_TOP_ITERATION_NUM = 5

    def __init__(self: any, project_path: str) -> None:
        self.project_path = project_path
        self.data = []

    def get_job_basic_info(self: any) -> list:
        """
        Get the info under the profiling job data: job info, device id, job name and collect time.
        :return: list of the basic data.
        """
        if InfoConfReader().is_host_profiling():
            device_id = Constant.NA
        else:
            device_id_list = InfoConfReader().get_device_list()
            if not device_id_list:
                error(self.FILE_NAME, "No device id found in the file of info.json, maybe the file is incomplete,"
                                      "please check the info.json under the directory: {0}".format(self.project_path))
                return []
            device_id = device_id_list[0]

        collection_time, _ = InfoConfReader().get_collect_date()
        if not collection_time:
            error(self.FILE_NAME, "No collection start time found in the file of start.info, maybe file is incomplete,"
                                  "please check the start.info under the directory: {0}".format(self.project_path))
            return []
        return [InfoConfReader().get_job_info(), device_id, os.path.basename(self.project_path),
                collection_time]

    def get_job_iteration_info(self: any) -> list:
        """
        Get the iteration info under the profiling job data: model id and index id.
        :return: list of the iteration data.
        """
        db_path_ge = PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_INFO)
        conn_ge, curs_ge = DBManager.check_connect_db_path(db_path_ge)

        db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_STEP_TRACE)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not conn_ge or not curs_ge or not DBManager.check_tables_in_db(
                db_path_ge, DBNameConstant.TABLE_GE_TASK):
            DBManager.destroy_db_connect(conn_ge, curs_ge)
            DBManager.destroy_db_connect(conn, curs)
            return []
        model_ids_set = self._get_model_id_set(curs_ge)

        if not conn or not curs or not DBManager.check_tables_in_db(
                db_path, DBNameConstant.TABLE_STEP_TRACE_DATA):
            DBManager.destroy_db_connect(conn_ge, curs_ge)
            DBManager.destroy_db_connect(conn, curs)
            return []
        iteration_infos = self._get_iteration_infos(curs)
        # filter model which contains no compute op
        iteration_infos_filtered = list(
            filter(lambda model_index: model_index[0] in model_ids_set, iteration_infos))

        iteration_infos_result = self._update_top_iteration_info(iteration_infos_filtered, model_ids_set, curs)
        DBManager.destroy_db_connect(conn_ge, curs_ge)
        DBManager.destroy_db_connect(conn, curs)

        return iteration_infos_result

    @classmethod
    def _get_iteration_infos(cls: any, curs: any) -> list:
        sql = "select model_id, max(index_id) from {} group by model_id".format(
            DBNameConstant.TABLE_STEP_TRACE_DATA)
        iteration_infos = DBManager.fetch_all_data(curs, sql)
        if not iteration_infos or not iteration_infos[0]:
            return []
        return iteration_infos

    @classmethod
    def _get_model_id_set(cls: any, curs_ge: any) -> set:
        sql = "select distinct model_id from {}".format(DBNameConstant.TABLE_GE_TASK)
        model_ids = DBManager.fetch_all_data(curs_ge, sql)
        model_ids_set = {model_id[0] for model_id in model_ids}
        return model_ids_set

    @classmethod
    def _update_top_iteration_info(cls: any, iteration_infos: list, model_ids_set: set, curs: any) -> list:
        if not iteration_infos or not model_ids_set:
            return []

        # get top {QUERY_TOP_ITERATION_NUM} time iterations of every model
        top_sql = \
            "select t.* from (" \
            "select index_id, model_id," \
            "(select count(*) + 1 from {0} as t2 where t2.model_id = t1.model_id and " \
            "(t2.step_end - t2.step_start) > (t1.step_end - t1.step_start)) as top " \
            "from {0} as t1) as t where top <= {1} order by model_id, top"\
                .format(DBNameConstant.TABLE_STEP_TRACE_DATA, cls.QUERY_TOP_ITERATION_NUM)
        top_index_ids = DBManager.fetch_all_data(curs, top_sql)
        if not top_index_ids or not top_index_ids[0]:
            return []
        top_index_ids_filtered = list(
            filter(lambda model_iter_info: model_iter_info[1] in model_ids_set, top_index_ids))

        # every iteration_info in format (model_id, max(index_id))
        # top_index_ids_filtered is in format [(index_id, model_id, top),...]
        iteration_info_list = []
        iteration_infos_result = []
        for iteration_info in iteration_infos:
            iteration_info_list = list(iteration_info)
            top_index_id = list(
                filter(lambda model_iter_info: model_iter_info[1] == iteration_info_list[0], top_index_ids_filtered))
            if not top_index_id:
                iteration_info_list.append(Constant.NA)
            else:
                top_index_ids_info = ",".join((str(id[0]) for id in top_index_id))
                iteration_info_list.append(top_index_ids_info)
            iteration_infos_result.append(iteration_info_list)
        return iteration_infos_result

    def assembly_job_info(self: any, basic_data: list, iteration_data: list) -> list:
        """
        Splicing data into query data object classes.
        :param basic_data: job info, device id, job name and collect time
        :param iteration_data: model id and index id
        :return: list of query data object
        """
        result = []
        if not iteration_data:
            # Model id and index id should be 'NA' without iteration data
            data = basic_data + [Constant.NA, Constant.NA, Constant.NA]
            query_bean = QueryDataBean(**dict(zip(self.QUERY_HEADERS, data)))
            result.append(query_bean)
            return result

        for _data in iteration_data:
            data = basic_data + list(_data)
            query_bean = QueryDataBean(**dict(zip(self.QUERY_HEADERS, data)))
            result.append(query_bean)
        return result

    def query_data(self: any) -> list:
        """
        Query data with basic info and iteration info.
        :return: list of query data object
        """
        basic_data = self.get_job_basic_info()
        if not basic_data:
            return []

        iteration_data = self.get_job_iteration_info()
        return self.assembly_job_info(basic_data, iteration_data)
