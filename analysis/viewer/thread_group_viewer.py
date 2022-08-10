#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import json
from collections import OrderedDict

from common_func.db_manager import ClassRowType
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_iteration import MsprofIteration
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from profiling_bean.db_dto.acl_dto import AclDto
from profiling_bean.db_dto.ge_op_execute_dto import GeOpExecuteDto
from profiling_bean.db_dto.ge_time_dto import GeTimeDto
from profiling_bean.db_dto.runtime_api_dto import RuntimeApiDto
from profiling_bean.prof_enum.data_tag import DataTag
from viewer.ge_info_report import get_ge_hash_dict


class ThreadGroupViewer:
    """
    viewer for acl,ge,runtime group by thread
    """
    PREFIX_ACL = "ACL@{}"
    PREFIX_GE = "GE@{}"
    PREFIX_GE_OP_EXECUTE = "GE_OP_EXECUTE@{}_{}"
    PREFIX_RUNTIME_API = "Runtime@{}"
    PREFIX_THREAD_ID = "Thread {}"

    def __init__(self: any, configs: any, params: dict) -> None:
        self._configs = configs
        self._params = params
        self._index_id = params.get(StrConstant.PARAM_ITER_ID)
        self._model_id = params.get(StrConstant.PARAM_MODEL_ID)
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._pid = InfoConfReader().get_json_pid_data()
        self._iteration = MsprofIteration(self._project_path)

    def get_timeline_data(self: any) -> str:
        """
        get timeline data
        :return: timeline json data
        """
        thread_data_map = {
            DataTag.ACL: self._get_acl_api_data,
            DataTag.GE_MODEL_TIME: self._get_ge_time_data,
            DataTag.GE_HOST: self._get_ge_op_execute_data,
            DataTag.RUNTIME_API: self._get_runtime_api_data
        }

        timeline_result = []
        for _tag in thread_data_map:
            timeline_result.extend(thread_data_map.get(_tag)())
        return json.dumps(timeline_result)

    def get_thread_data(self: any, db_name: str, table_name: str, sql: str, dto: any) -> list:
        """
        get thread data
        :param db_name:db name
        :param table_name:table name
        :param sql: sql
        :param dto: dto for data
        :return:
        """
        conn, curs = DBManager.check_connect_db(self._project_path, db_name)
        if not DBManager.judge_table_exist(curs, table_name):
            return []
        curs.row_factory = ClassRowType.class_row(dto)
        return DBManager.fetch_all_data(curs, sql)

    def _get_acl_api_sql(self):
        where_condition = self._iteration.get_condition_within_iteration(self._index_id, self._model_id,
                                                                         time_start_key='start_time',
                                                                         time_end_key='end_time')
        return f"select api_name,api_type,start_time,end_time,thread_id " \
               f"from {DBNameConstant.TABLE_ACL_DATA} {where_condition}"

    def _get_acl_api_data(self: any) -> list:
        _format_data = []
        sql = self._get_acl_api_sql()
        acl_data = self.get_thread_data(DBNameConstant.DB_ACL_MODULE, DBNameConstant.TABLE_ACL_DATA, sql, AclDto)

        if not acl_data:
            return []
        for _data in acl_data:
            args = OrderedDict([("Mode", _data.api_type)])
            _format_data.append([self.PREFIX_ACL.format(_data.api_name), self._pid,
                                 self.PREFIX_THREAD_ID.format(_data.thread_id),
                                 _data.start_time / NumberConstant.NS_TO_US,
                                 (_data.end_time - _data.start_time) / NumberConstant.NS_TO_US,
                                 args
                                 ])
        return TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, _format_data)

    def _get_ge_time_sql(self):
        where_condition = self._iteration.get_condition_within_iteration(self._index_id, self._model_id,
                                                                         time_start_key='infer_start',
                                                                         time_end_key='infer_end')
        return f"select model_name,model_id,thread_id,input_start,input_end,infer_start,infer_end,output_start," \
               f"output_end from {DBNameConstant.TABLE_GE_MODEL_TIME} {where_condition}"

    def _get_ge_time_data(self: any) -> list:
        _format_data = []
        sql = self._get_ge_time_sql()
        ge_time_data = self.get_thread_data(DBNameConstant.DB_GE_MODEL_TIME, DBNameConstant.TABLE_GE_MODEL_TIME, sql,
                                            GeTimeDto)
        if not ge_time_data:
            return []
        for _ge_time_data in ge_time_data:
            args = OrderedDict([("Model Name", _ge_time_data.model_name),
                                ("Model ID", _ge_time_data.model_id)])
            _input_data = [self.PREFIX_GE.format("Input"), self._pid,
                           self.PREFIX_THREAD_ID.format(_ge_time_data.thread_id),
                           _ge_time_data.input_start / NumberConstant.NS_TO_US,
                           (_ge_time_data.input_end - _ge_time_data.input_start) / NumberConstant.NS_TO_US,
                           args
                           ]
            _infer_data = [self.PREFIX_GE.format("Infer"), self._pid,
                           self.PREFIX_THREAD_ID.format(_ge_time_data.thread_id),
                           _ge_time_data.infer_start / NumberConstant.NS_TO_US,
                           (_ge_time_data.infer_end - _ge_time_data.infer_start) / NumberConstant.NS_TO_US,
                           args
                           ]
            _output_data = [self.PREFIX_GE.format("Output"), self._pid,
                            self.PREFIX_THREAD_ID.format(_ge_time_data.thread_id),
                            _ge_time_data.output_start / NumberConstant.NS_TO_US,
                            (_ge_time_data.output_end - _ge_time_data.output_start) / NumberConstant.NS_TO_US,
                            args
                            ]
            _format_data.extend([_input_data, _infer_data, _output_data])
        return TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, _format_data)

    def _get_ge_op_execute_sql(self):
        where_condition = self._iteration.get_condition_within_iteration(self._index_id, self._model_id,
                                                                         time_start_key='start_time',
                                                                         time_end_key='end_time')
        return f"select thread_id,op_type,event_type,start_time,end_time " \
               f"from {DBNameConstant.TABLE_GE_HOST} {where_condition}"

    def _get_ge_op_execute_data(self: any) -> list:
        _format_data = []
        hash_dict = get_ge_hash_dict(self._project_path)
        sql = self._get_ge_op_execute_sql()
        ge_op_execute_data = self.get_thread_data(DBNameConstant.DB_GE_HOST_INFO, DBNameConstant.TABLE_GE_HOST, sql,
                                                  GeOpExecuteDto)
        if not ge_op_execute_data:
            return []
        for _data in ge_op_execute_data:
            _format_data.append([self.PREFIX_GE_OP_EXECUTE.format(
                hash_dict.get(_data.op_type, _data.op_type), hash_dict.get(_data.event_type, _data.event_type)),
                self._pid, self.PREFIX_THREAD_ID.format(_data.thread_id),
                _data.start_time / NumberConstant.NS_TO_US,
                (_data.end_time - _data.start_time) / NumberConstant.NS_TO_US])
        return TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TASK_TIME_GRAPH_HEAD, _format_data)

    def _get_runtime_api_sql(self):
        where_condition = self._iteration.get_condition_within_iteration(self._index_id, self._model_id,
                                                                         time_start_key='entry_time',
                                                                         time_end_key='exit_time')
        return f"select entry_time,exit_time,api,thread from {DBNameConstant.TABLE_API_CALL} {where_condition}"

    def _get_runtime_api_data(self: any) -> list:
        _format_data = []
        sql = self._get_runtime_api_sql()
        runtime_api_data = self.get_thread_data(DBNameConstant.DB_RUNTIME, DBNameConstant.TABLE_API_CALL, sql,
                                                RuntimeApiDto)
        if not runtime_api_data:
            return []
        for _runtime_api_data in runtime_api_data:
            _format_data.append([self.PREFIX_RUNTIME_API.format(_runtime_api_data.api),
                                 self._pid, self.PREFIX_THREAD_ID.format(_runtime_api_data.thread),
                                 _runtime_api_data.entry_time / NumberConstant.NS_TO_US,
                                 (_runtime_api_data.exit_time - _runtime_api_data.entry_time) / NumberConstant.NS_TO_US
                                 ])
        return TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TASK_TIME_GRAPH_HEAD, _format_data)
