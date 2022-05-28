#!/usr/bin/python3
# coding:utf-8
"""
This scripts is used to provide for time line page
Copyright Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
"""
import json
import logging
import sqlite3
from collections import OrderedDict

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_iteration import MsprofIteration
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.utils import Utils


class HCCLExport:
    """
    hccl export
    """

    def __init__(self: any, param: dict) -> None:
        self.project_path = param.get(StrConstant.PARAM_RESULT_DIR)
        self.result = []
        self.model_id = param.get(StrConstant.PARAM_MODEL_ID)
        self.iter_id = param.get(StrConstant.PARAM_ITER_ID)
        self.pid_value = InfoConfReader().get_json_pid_data()

    def get_hccl_timeline_data(self: any) -> str:
        """
        get data for hccl timeline
        """
        sql = self.get_hccl_sql()
        hccl_data = self._get_hccl_sql_data(sql)
        if hccl_data:
            self._format_hccl_data(hccl_data)
        return json.dumps(self.result)

    def get_hccl_sql(self: any) -> str:
        """
        get hccl query sql
        :return: control statement
        """
        sql = "select name,plane_id,timestamp,duration,bandwidth,stream_id," \
              "task_id, task_type,transport_type,size,stage,step from {0}".format(DBNameConstant.TABLE_HCCL_ALL_REDUCE)

        if not ProfilingScene().is_operator():
            iter_time = MsprofIteration(self.project_path).get_iteration_time(self.iter_id, self.model_id)
            if iter_time:
                sql = "select name,plane_id,timestamp,duration,bandwidth,stream_id," \
                      "task_id, task_type,transport_type,size,stage,step from {0} where timestamp>={1} " \
                      "and timestamp<{2}".format(DBNameConstant.TABLE_HCCL_ALL_REDUCE,
                                                 iter_time[0][0], iter_time[0][1])
        return sql

    def _get_meta_data(self: any, hccl_data: list) -> None:
        self.result = TraceViewManager.metadata_event(
            [["process_name", self.pid_value, InfoConfReader().get_json_tid_data(), "HCCL"]])
        tid_list_set = set(map(lambda x: x[1], hccl_data))
        tid_list = sorted(tid_list_set)
        self.result.extend(TraceViewManager.metadata_event(
            Utils.generator_to_list(["thread_name", self.pid_value, tid_value, "Plane {}".format(tid_value)]
                                    for tid_value in tid_list)))

    def _format_hccl_data(self: any, hccl_data: list) -> None:
        self._get_meta_data(hccl_data)
        _hccl_format_data = [0] * 2 * len(hccl_data)
        for index, _hccl_data in enumerate(hccl_data):
            hccl_args = OrderedDict([("Bandwidth(GB/s)", _hccl_data[4]),
                                     ("Stream Id", _hccl_data[5]),
                                     ("Task Id", _hccl_data[6]),
                                     ("Task Type", _hccl_data[7]),
                                     ("Transport Type", _hccl_data[8]),
                                     ("Size(Byte)", _hccl_data[9]),
                                     ("Stage", _hccl_data[10]),
                                     ("Step", _hccl_data[11])])
            _hccl_data_pice = [_hccl_data[0], self.pid_value, _hccl_data[1],
                               _hccl_data[2], _hccl_data[3], hccl_args]
            _hccl_stage_pice = ["Stage{}Step{}".format(_hccl_data[10], _hccl_data[11]), self.pid_value, _hccl_data[1],
                                _hccl_data[2], _hccl_data[3], hccl_args]
            _hccl_format_data[index] = _hccl_data_pice
            _hccl_format_data[index + len(hccl_data)] = _hccl_stage_pice
        self.result.extend(TraceViewManager.time_graph_trace(
            TraceViewHeaderConstant.GRPC_TIME_GRAPH_HEAD, _hccl_format_data))

    def _get_hccl_sql_data(self: any, sql: str) -> list:
        conn, cur = DBManager.check_connect_db(self.project_path, DBNameConstant.DB_HCCL)
        try:
            if not self._check_hccl_table(conn, cur):
                return []
        except sqlite3.Error as error:
            logging.error(str(error), exc_info=Constant.TRACE_BACK_SWITCH)
            return []
        else:
            sql_datas = DBManager.fetch_all_data(cur, sql)
            if not sql_datas:
                self.result = {'status': NumberConstant.WARN,
                               "info": "get hccl data failed,"
                                       " may be lack of hccl files containing iteration {}.".format(self.iter_id)}
            return sql_datas
        finally:
            DBManager.destroy_db_connect(conn, cur)

    def _check_hccl_table(self: any, conn: any, curs: any) -> bool:
        if not (conn and curs) or not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_HCCL_ALL_REDUCE):
            self.result = {'status': NumberConstant.ERROR,
                           "info": "get hccl data failed, may be the hccl file not completed or hccl parser failed."
                                   " please check data file."}
            return False
        return True
