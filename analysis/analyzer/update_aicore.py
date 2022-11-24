#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.

import logging
import os

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from common_func.msprof_iteration import MsprofIteration
from common_func.path_manager import PathManager
from profiling_bean.db_dto.ge_task_dto import GeTaskDto


class UpdateAICoreData:
    """
    parsing Update ai core data class
    """
    INVALID_CORE_NUM = {'aic': 0, 'aiv': 0}
    INVALID_FREQ = 0
    TASK_ID_KEY = "task_id"
    STREAM_ID_KEY = "stream_id"
    STREAM_TASK_KEY_FMT = "{0}-{1}"
    TOTAL_CYCLES_KEY = "total_cycles"
    AIC_TOTAL_CYCLES_KEY = "aic_total_cycles"
    AIV_TOTAL_CYCLES_KEY = "aiv_total_cycles"
    CORE_TYPE = {1: "aiv", 0: 'aic'}

    def __init__(self: any, sample_config: dict) -> None:
        self.sample_config = sample_config
        self.host_id = None
        self.device_id = None
        self.project_path = None
        self.sql_dir = None
        self.warning_cnt = 0
        self._block_dims = {'block_dim': {}, 'mix_block_dim': {}}

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
        try:
            self.__update_ai_core_data()
        except ProfException:
            logging.error('Updating aicore data failed')
        if self.warning_cnt > 0:
            logging.warning("Can not find the stream id and task id from ge data when getting block, "
                            "maybe ge data has lost %d times", self.warning_cnt)

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
                                            DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY) or \
               DBManager.check_tables_in_db(PathManager.get_db_path(self.project_path, DBNameConstant.DB_RUNTIME),
                                            DBNameConstant.TABLE_AIV_METRIC_SUMMARY)

    def __update_ai_core_data(self: any) -> None:
        """
        calculate updated ai core data, two steps are included here:
        1.calculate total time using the following formula
        task_cyc*1000/(freq)/block_num*((block_num+core_num-1)//core_num)
        2.calculate vector/scalar/mte time from corresponding ratio
        """
        self.__get_block_dim_from_ge()
        if not self._block_dims.get('block_dim'):
            logging.warning("Unable to get block dim to calculate ai core time")
            return
        core_num, freq = self.__get_config_params()
        if core_num == self.INVALID_CORE_NUM or freq == self.INVALID_FREQ:
            logging.error("Unable to get core number or ai core frequency")
            return
        self.__update_ai_core_db(core_num, freq)

    def __format_ge_data(self: any, ge_data: list) -> None:
        for data in ge_data:
            if data.task_type not in [Constant.TASK_TYPE_AI_CORE, Constant.TASK_TYPE_AIV,
                                      Constant.TASK_TYPE_MIX_AIC, Constant.TASK_TYPE_MIX_AIV]:
                continue
            _key = self.STREAM_TASK_KEY_FMT.format(data.task_id, data.stream_id)
            self._block_dims['block_dim'].setdefault(_key, []).append(int(data.block_dim))
            if data.task_type in [Constant.TASK_TYPE_MIX_AIV, Constant.TASK_TYPE_MIX_AIC]:
                self._block_dims['mix_block_dim'].setdefault(_key, []).append(int(data.mix_block_dim))

    def __get_block_dim_from_ge(self: any) -> None:
        """
        get ge data from ge info in the format of [task_id, stream_id, blockid]
        :return: {"task_id-stream_id":blockdim}
        """
        if not DBManager.check_tables_in_db(self.get_db_path(DBNameConstant.DB_GE_INFO), DBNameConstant.TABLE_GE_TASK):
            return
        ge_conn, ge_curs = DBManager.check_connect_db_path(
            os.path.join(self.sql_dir, DBNameConstant.DB_GE_INFO))
        if ge_conn and ge_curs:
            ge_data = self.__get_block_dim_data(ge_curs)
            if not ge_data:
                DBManager.destroy_db_connect(ge_conn, ge_curs)
                return
            self.__format_ge_data(ge_data)
        DBManager.destroy_db_connect(ge_conn, ge_curs)

    def __get_block_dim_data(self: any, ge_curs: any) -> list:
        if ProfilingScene().is_operator():
            sql = "select task_id, stream_id, task_type, block_dim, mix_block_dim from {0} " \
                  "order by timestamp".format(DBNameConstant.TABLE_GE_TASK)
            return DBManager.fetch_all_data(ge_curs, sql, dto_class=GeTaskDto)
        ge_data = []
        index_id = self.sample_config.get("iter_id", NumberConstant.DEFAULT_ITER_ID)
        model_id = self.sample_config.get("model_id", NumberConstant.DEFAULT_MODEL_ID)
        iter_list = MsprofIteration(self.project_path).get_iter_list_with_index_and_model(index_id, model_id)
        sql = "select task_id, stream_id, task_type, block_dim, mix_block_dim from {0} " \
              "where model_id=? and (index_id=0 or index_id=?) " \
              " order by timestamp".format(
            DBNameConstant.TABLE_GE_TASK)
        for iter_id, model_id in iter_list:
            ge_data.extend(DBManager.fetch_all_data(ge_curs, sql, (model_id, iter_id), dto_class=GeTaskDto))
        return ge_data

    def __get_config_params(self: any) -> tuple:
        """
        get core num and ai core freq from config file
        """
        core_num = {'aic': 0, 'aiv': 0}
        for file_name in os.listdir(self.project_path):
            matched_name = FileManager.is_info_json_file(file_name)
            if matched_name:
                ai_core_num = InfoConfReader().get_data_under_device("ai_core_num")
                aiv_num = InfoConfReader().get_data_under_device("aiv_num")
                core_num['aic'] = ai_core_num if ai_core_num else 24
                core_num['aiv'] = aiv_num if aiv_num else 48
                if not core_num:
                    break
                freq = InfoConfReader().get_freq(StrConstant.AIC)
                return core_num, freq
        # set invalid core num and ai core freq to 0
        return core_num, self.INVALID_FREQ

    def __update_ai_core_db(self: any, core_num: any, freq: any) -> None:
        db_path = self.get_db_path(DBNameConstant.DB_RUNTIME)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not (conn and curs):
            logging.error("Failed to update aicore data. Cannot connect to the database. Aicore data may be lost.")
            return
        tables = DBManager.fetch_all_data(curs, "select name from sqlite_master where type='table'")
        if tables:
            tables = list(map(lambda x: x[0], tables))
        if DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY in tables:
            self.__update_ai_core_table(conn, DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY, core_num,
                                        freq)
        if DBNameConstant.TABLE_AIV_METRIC_SUMMARY in tables:
            self.__update_ai_core_table(conn, DBNameConstant.TABLE_AIV_METRIC_SUMMARY, core_num,
                                        freq)
        DBManager.destroy_db_connect(conn, curs)

    def __update_ai_core_table(self: any, conn: any, table_name: str, *params: tuple) -> None:
        core_num, freq = params
        headers, data = self.__get_ai_core_data(conn.cursor(), table_name)
        if not (headers and data):
            return
        if "total_time" not in headers:
            data = self.__cal_total(headers, data, core_num, freq)
            headers = ["total_time", "aiv_time"] + headers
        headers, data = self.__add_pipe_time(headers, data)
        self.__update_ai_core(conn, headers, data, table_name)

    def __cal_total(self: any, headers: any, ai_core_data: list, *config_infos: any) -> list:
        """
        calculate total time if total time does not exist
        :param ai_core_data: ai core data selected from metric summary [...,task_id,stream_id,iterid]
        :param block_dims: block dim dict selected from ge_task data
        :param core_num: ai core number
        :param freq: ai core frequency
        :return:
        """
        core_num_dict, freq = config_infos
        task_id_index, stream_id_index, cycle_index, mix_cycle_index = self.__get_col_indexes_to_cal_total(headers)
        if not ai_core_data:
            logging.error("unable to get AI Core data.")
            return []
        time_data = []
        time_sum = 0
        for item in ai_core_data:
            mix_total_time, total_time = 0, 0
            core_type = self.CORE_TYPE.get(item[-1], 'aic')
            core_num = core_num_dict.get(core_type, 0)
            block = self._get_current_block('block_dim', item, task_id_index, stream_id_index)
            if mix_cycle_index is not None:
                mix_block = self._get_current_block('mix_block_dim', item, task_id_index, stream_id_index)
                mix_core_num = core_num_dict.get(self._get_mix_core_type(item), 0)
                if core_type == 'aiv':
                    core_num, block, mix_core_num, mix_block = mix_core_num, mix_block, core_num, block
                if mix_block and mix_core_num:
                    mix_total_time = item[mix_cycle_index] * 1000 / int(freq) / mix_block * \
                                     ((mix_block + mix_core_num - 1) // mix_core_num)
                    mix_total_time = round(mix_total_time, NumberConstant.DECIMAL_ACCURACY)
            if block:
                if int(freq) == 0 or core_num == 0:
                    total_time = 0
                else:
                    total_time = item[cycle_index] * 1000 / int(freq) / block * \
                                 ((block + core_num - 1) // core_num)
                    total_time = round(total_time, NumberConstant.DECIMAL_ACCURACY)
            time_data.append([total_time, mix_total_time] + list(item))

        for item in time_data:
            if item[0] == 0:
                item[0] = time_sum
                break
        return time_data

    @staticmethod
    def _get_mix_core_type(ai_core_data: list) -> str:
        mix_core_type = {'4-6': 'aiv', '4-7': 'aic'}
        # data[-2]: ffts_type,  data[-5]: subtask_type
        return mix_core_type.get('{}-{}'.format(ai_core_data[-2], ai_core_data[-5]))

    def _get_current_block(self: any, block_type: str, ai_core_data: list, task_id_index: any,
                           stream_id_index: any) -> any:
        """
        get the current block dim when stream id and task id occurs again
        :param ai_core_data: ai core pmu data
        :param task_id_index: index for task id
        :param stream_id_index: index for stream id
        :return: block dim
        """
        block = self._block_dims.get(block_type, {}).get(self.STREAM_TASK_KEY_FMT.format(ai_core_data[task_id_index],
                                                                                          ai_core_data[
                                                                                              stream_id_index]))
        if not block:
            self.warning_cnt += 1
            return 0
        return block.pop(0) if len(block) > 1 else block[0]

    def __get_col_indexes_to_cal_total(self: any, headers: any) -> tuple:
        infer_cols = {self.TASK_ID_KEY, self.STREAM_ID_KEY, self.TOTAL_CYCLES_KEY}
        if infer_cols.issubset(headers):
            res_tuple = headers.index(self.TASK_ID_KEY), \
                        headers.index(self.STREAM_ID_KEY), \
                        headers.index(self.TOTAL_CYCLES_KEY), \
                        None
            return res_tuple
        mix_cols = {self.TASK_ID_KEY, self.STREAM_ID_KEY, self.AIC_TOTAL_CYCLES_KEY, self.AIV_TOTAL_CYCLES_KEY}
        if mix_cols.issubset(headers):
            res_tuple = headers.index(self.TASK_ID_KEY), \
                        headers.index(self.STREAM_ID_KEY), \
                        headers.index(self.AIC_TOTAL_CYCLES_KEY), \
                        headers.index(self.AIV_TOTAL_CYCLES_KEY)
            return res_tuple
        logging.warning(
            "unable to find task id or stream id or total cycles in original ai core data.")
        raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

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
