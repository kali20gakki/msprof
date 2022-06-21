#!/usr/bin/python3
# coding:utf-8
"""
This script is used to update aicore metric summary table
Copyright Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
"""

import logging
import os
import sqlite3

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager


class UpdateAICoreData:
    """
    parsing Update ai core data class
    """
    INVALID_CORE_NUM = 0
    INVALID_FREQ = 0
    TASK_ID_TRAIN_KEY = "task_id"
    STREAM_ID_TRAIN_KEY = "stream_id"
    TASK_ID_INFER_KEY = "task_id"
    STREAM_ID_INFER_KEY = "stream_id"
    STREAM_TASK_KEY_FMT = "{0}-{1}"
    TOTAL_CYCLES_KEY = "total_cycles"

    def __init__(self: any, sample_config: dict) -> None:
        self.sample_config = sample_config
        self.host_id = None
        self.device_id = None
        self.project_path = None
        self.sql_dir = None

    @staticmethod
    def __add_pipe_time(headers: list, ai_core_data: list) -> tuple:
        """
        add vec_time/scalar_time/mac_time/mte time into ai core data
        :param headers:original data header
        :param ai_core_data: [total_time, total_cycle,...]
        :return:
        """
        ratio_index_list = []
        # modify headers
        new_headers = []
        new_data = []
        for header in headers:
            if header in Constant.AICORE_METRICS_LIST.get("PipeUtilization").split(",")[:-1] \
                    and len(header) > NumberConstant.RATIO_NAME_LEN:
                # header should be in the format "xxx_ratio"
                new_headers.append(header[:-NumberConstant.RATIO_NAME_LEN] + "time")
                new_headers.append(header)
                ratio_index_list.append(headers.index(header))
            else:
                new_headers.append(header)
        if not ratio_index_list:
            return new_headers, ai_core_data
        # formulate timeline data
        for item in ai_core_data:
            tmp = list(item)
            for ratio_index in ratio_index_list[::-1]:
                tmp.insert(ratio_index, round(item[0] * item[ratio_index], NumberConstant.DECIMAL_ACCURACY))
            new_data.append(tmp)
        return new_headers, new_data

    @staticmethod
    def __get_ai_core_data(curs: any, table_name: str) -> tuple:
        """
        get aicore metric data
        :param curs: ai core db connection cursor
        :param table_name: ai core table name
        :return:
        """
        headers = DBManager.get_filtered_table_headers(curs, table_name, *Constant.AICORE_PIPE_LIST)
        aicore_data = DBManager.fetch_all_data(curs, "select {1} from {0} ".format(table_name,
                                                                                   ",".join(headers)))
        return headers, aicore_data

    @staticmethod
    def __get_create_sql_str(curs: any, headers: list, table_name: str) -> str:
        cols_infos = []
        cols_with_type = DBManager.get_table_info(curs, table_name)
        for header in headers:
            if header in cols_with_type:
                cols_infos.append("[{}] {}".format(header, cols_with_type[header]))
            else:
                cols_infos.append("[{}] {}".format(header, "numeric"))
        return ",".join(cols_infos)

    def run(self: any) -> None:
        """
        update ai core time entry.
        """
        try:
            if not self.__init_param():
                return
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        if not self.__check_required_tables_exist():
            logging.warning("Unable to get required data to update ai core data.")
            return
        self.__update_ai_core_data()

    def get_db_path(self: any, db_name: str) -> str:
        """
        get database path
        :param db_name: database name
        :return: DB path
        """
        return os.path.join(self.sql_dir, db_name)

    def __init_param(self: any) -> bool:
        """
        initialize params
        """
        self.project_path = self.sample_config.get("result_dir", "")
        if self.project_path is None or not os.path.exists(self.project_path):
            return False
        self.sql_dir = PathManager.get_sql_dir(self.project_path)
        return True

    def __check_required_tables_exist(self: any) -> bool:
        """
        check exist of ge db data and ai core data
        """
        return self.__check_ge_table_exist() and self.__check_ai_core_table_exist()

    def __check_ge_table_exist(self: any) -> bool:
        return DBManager.check_tables_in_db(self.get_db_path(DBNameConstant.DB_GE_INFO), DBNameConstant.TABLE_GE_TASK)

    def __check_ai_core_table_exist(self: any) -> bool:
        return DBManager.check_tables_in_db(PathManager.get_db_path(self.project_path, DBNameConstant.DB_RUNTIME),
                                            DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY)

    def __update_ai_core_data(self: any) -> None:
        """
        calculate updated ai core data, two steps are included here:
        1.calculate total time using the following formula
        task_cyc*1000/(freq)/block_num*((block_num+core_num-1)//core_num)
        2.calculate vector/scalar/mte time from corresponding ratio
        """
        block_dims = self.__get_block_dim_from_ge()
        if not block_dims:
            logging.warning("Unable to get block dim to calculate ai core time")
            return
        core_num, freq = self.__get_config_params()
        if core_num == self.INVALID_CORE_NUM or freq == self.INVALID_FREQ:
            logging.error("Unable to get core number or ai core frequency")
            return
        self.__update_ai_core_db(block_dims, core_num, freq)

    def __format_ge_data(self: any, ge_data: list, res_data: dict) -> None:
        for data in ge_data:
            _key = self.STREAM_TASK_KEY_FMT.format(*data[:2])
            res_data.setdefault(_key, []).append(int(data[2]))

    def __get_block_dim_from_ge(self: any) -> dict:
        """
        get ge data from ge info in the format of [task_id, stream_id, blockid]
        :return: {"task_id-stream_id":blockdim}
        """
        if not DBManager.check_tables_in_db(self.get_db_path(DBNameConstant.DB_GE_INFO), DBNameConstant.TABLE_GE_TASK):
            return {}
        res_data = {}
        ge_conn, ge_curs = DBManager.check_connect_db_path(
            os.path.join(self.sql_dir, DBNameConstant.DB_GE_INFO))
        if ge_conn and ge_curs:
            block_dim_sql, iter_id = self.__get_block_dim_sql()
            ge_data = DBManager.fetch_all_data(ge_curs, block_dim_sql, iter_id)
            if not ge_data:
                DBManager.destroy_db_connect(ge_conn, ge_curs)
                return res_data
            self.__format_ge_data(ge_data, res_data)
        DBManager.destroy_db_connect(ge_conn, ge_curs)
        return res_data

    def __get_block_dim_sql(self: any) -> tuple:
        sql = "select task_id, stream_id, block_dim from {0} where task_type='{1}' " \
              "order by timestamp".format(DBNameConstant.TABLE_GE_TASK, Constant.TASK_TYPE_AI_CORE)
        if not ProfilingScene().is_operator():
            iter_id = self.sample_config.get("iter_id", NumberConstant.DEFAULT_ITER_ID)
            model_id = self.sample_config.get("model_id", NumberConstant.DEFAULT_MODEL_ID)
            sql = "select task_id, stream_id, block_dim from {0} " \
                  "where model_id=? and (index_id=0 or index_id=?) " \
                  "and task_type='{1}' order by timestamp".format(
                DBNameConstant.TABLE_GE_TASK, Constant.TASK_TYPE_AI_CORE)
            return sql, (model_id, iter_id,)
        return sql, ()

    def __get_config_params(self: any) -> tuple:
        """
        get core num and ai core freq from config file
        """
        for file_name in os.listdir(self.project_path):
            matched_name = FileManager.is_info_json_file(file_name)
            if matched_name:
                core_num = InfoConfReader().get_data_under_device("ai_core_num")
                if not core_num:
                    break
                freq = InfoConfReader().get_freq(StrConstant.AIC)
                return core_num, freq
        # set invalid core num and ai core freq to 0
        return self.INVALID_CORE_NUM, self.INVALID_FREQ

    def __update_ai_core_db(self: any, block_dims: any, core_num: any, freq: any) -> None:
        db_path = self.get_db_path(DBNameConstant.DB_RUNTIME)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not (conn and curs):
            logging.error("Failed to update aicore data. Cannot connect to the database. Aicore data may be lost.")
            return
        tables = DBManager.fetch_all_data(curs, "select name from sqlite_master where type='table'")
        if tables:
            tables = list(map(lambda x: x[0], tables))
        if DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY in tables:
            self.__update_ai_core_table(conn, DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY, block_dims, core_num,
                                        freq)
        DBManager.destroy_db_connect(conn, curs)

    def __update_ai_core_table(self: any, conn: any, table_name: str, block_dims: any, *params: tuple) -> None:
        core_num, freq = params
        headers, data = self.__get_ai_core_data(conn.cursor(), table_name)
        if not (headers and data):
            return
        if "total_time" not in headers:
            data = self.__cal_total(headers, data, block_dims, core_num, freq)
            headers = ["total_time"] + headers
        headers, data = self.__add_pipe_time(headers, data)
        self.__update_ai_core(conn, headers, data, table_name)

    def __cal_total(self: any, headers: any, ai_core_data: list, block_dims: any, *config_infos: any) -> list:
        """
        calculate total time if total time does not exist
        :param ai_core_data: ai core data selected from metric summary [...,task_id,stream_id,iterid]
        :param block_dims: block dim dict selected from ge_task data
        :param core_num: ai core number
        :param freq: ai core frequency
        :return:
        """
        core_num, freq = config_infos
        task_id_index, stream_id_index, cycle_index = self.__get_col_indexes_to_cal_total(headers)
        if not ai_core_data or task_id_index == Constant.INVALID_INDEX or stream_id_index == Constant.INVALID_INDEX or \
                cycle_index == Constant.INVALID_INDEX:
            logging.error("unable to get AI Core data.")
            return []
        time_data = []
        time_sum = 0
        for item in ai_core_data:
            block = self._get_current_block(block_dims, item, task_id_index, stream_id_index)
            if block:
                if int(freq) == 0 or core_num == 0:
                    total_time = 0
                else:
                    total_time = item[cycle_index] * 1000 / int(freq) / block * \
                                 ((block + core_num - 1) // core_num)
                    total_time = round(total_time, NumberConstant.DECIMAL_ACCURACY)
                    time_sum += total_time
            else:
                total_time = 0
            time_data.append([total_time] + list(item))

        for item in time_data:
            if item[0] == 0:
                item[0] = time_sum
                break
        return time_data

    def _get_current_block(self: any, block_dims: any, ai_core_data: list, task_id_index: any,
                           stream_id_index: any) -> any:
        """
        get the current block dim when stream id and task id occurs again
        :param block_dims: block dims
        :param ai_core_data: ai core pmu data
        :param task_id_index: index for task id
        :param stream_id_index: index for stream id
        :return: block dim
        """
        block = block_dims.get(self.STREAM_TASK_KEY_FMT.format(ai_core_data[task_id_index],
                                                               ai_core_data[stream_id_index]))
        if not block:
            logging.error("Can not find the stream id and task id from ge data, "
                          "maybe ge data has lost: %s", self.STREAM_TASK_KEY_FMT.format(ai_core_data[task_id_index],
                                                                                        ai_core_data[stream_id_index]))
            return 0
        return block.pop(0) if len(block) > 1 else block[0]

    def __get_col_indexes_to_cal_total(self: any, headers: any) -> tuple:
        infer_cols = {self.TASK_ID_INFER_KEY, self.STREAM_ID_INFER_KEY, self.TOTAL_CYCLES_KEY}
        if infer_cols.issubset(headers):
            return headers.index(self.TASK_ID_INFER_KEY), \
                   headers.index(self.STREAM_ID_INFER_KEY), \
                   headers.index(self.TOTAL_CYCLES_KEY)
        train_cols = {self.TASK_ID_TRAIN_KEY, self.STREAM_ID_TRAIN_KEY, self.TOTAL_CYCLES_KEY}
        if train_cols.issubset(headers):
            return headers.index(self.TASK_ID_TRAIN_KEY), \
                   headers.index(self.STREAM_ID_TRAIN_KEY), \
                   headers.index(self.TOTAL_CYCLES_KEY)
        logging.error(
            "unable to find task id or stream id or total cycles in original ai core data.")
        return Constant.INVALID_INDEX, Constant.INVALID_INDEX, Constant.INVALID_INDEX

    def __update_ai_core(self: any, conn: any, headers: list, data: any, table_name: str) -> None:
        """
        update aicore metric summary table
        :param conn: ai core connection cursor
        :param headers:data header
        :param data: [total_time, total_cycle,...]
        :param table_name: ai core table name
        :return:
        """

        create_sql_str = self.__get_create_sql_str(conn.cursor(), headers, table_name)
        # drop existed metric summary table
        DBManager.drop_table(conn, table_name)

        sql = "create table {} (".format(table_name) + create_sql_str + ")"
        DBManager.execute_sql(conn, sql)
        DBManager.insert_data_into_table(conn, table_name, data)
