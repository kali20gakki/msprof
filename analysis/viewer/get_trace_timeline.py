#!/usr/bin/python3
# coding=utf-8
"""
This script is used to generate json for trace-viewer
Copyright Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
"""

import json
import logging
import os
import sqlite3
from collections import OrderedDict

from common_func.common import CommonConstant
from common_func.common import warn
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.json_manager import JsonManager
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.utils import Utils
from viewer.get_msvp_summary import get_aicore_utilization
from viewer.get_msvp_timeline_training import get_dvpp_engine_id
from viewer.get_msvp_timeline_training import get_dvpp_ids
from viewer.get_msvp_timeline_training import get_dvpp_legend
from viewer.get_msvp_timeline_training import get_dvpp_total_data


class TraceViewer:
    """
    Trace viewer object
    """

    def __init__(self: any, scope: str) -> None:
        self.scope = scope

    @staticmethod
    def multiple_name_dump(trace_data: dict, legend: dict, delta_dev: any,
                           pid: int = TraceViewHeaderConstant.DEFAULT_PID_VALUE,
                           tid: int = TraceViewHeaderConstant.DEFAULT_TID_VALUE) -> list:
        """
        Multiple task id
        :param trace_data: trace data
        :param legend: legend
        :param delta_dev: delta time for device
        :param pid: pid
        :param tid: tid
        :return: result
        """
        _result = []
        if not isinstance(trace_data, dict) or not trace_data:
            return _result
        for key in list(trace_data.keys()):
            column_trace_data = Utils.generator_to_list(
                [key, TraceViewer._cal_sys_time_us(delta_dev, item), pid, tid,
                 OrderedDict(list(zip(legend[key], item[1:])))] for item in trace_data[key])
            _result += \
                TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, column_trace_data)
        return _result

    @staticmethod
    def format_trace_data(data_item: list, trace_name: str, timestamp: any, pid: int, tid: int) -> tuple:
        """
        PCIe data format
        :param data_item: data
        :param trace_name: tarce name
        :param timestamp: time stamp
        :param pid: pid
        :param tid: tid
        :return: result
        """
        tmp_args_data = {"Tx": 0,
                         "Rx": 0}
        if trace_name == "PCIe_post":
            tmp_args_data["Tx"] = data_item[4]
            tmp_args_data["Rx"] = data_item[-7]
        elif trace_name == "PCIe_nonpost":
            tmp_args_data["Tx"] = data_item[7]
            tmp_args_data["Rx"] = data_item[-4]
        elif trace_name == "PCIe_cpl":
            tmp_args_data["Tx"] = data_item[10]
            tmp_args_data["Rx"] = data_item[-1]
        else:
            tmp_args_data["Tx"] = data_item[13]
            tmp_args_data["Rx"] = 0
        tmp_trace_data = (trace_name, timestamp, pid, tid, tmp_args_data)
        return tmp_trace_data

    @staticmethod
    def format_trace_events(trace_evnets: list) -> str:
        """
        Format traceEvents
        :param trace_evnets:trace events
        :return: result
        """
        return json.dumps(trace_evnets)

    @staticmethod
    def _cal_sys_time_us(delta_dev: any, item: list) -> int:
        return int(float(item[0]) + delta_dev * NumberConstant.NANO_SECOND) / NumberConstant.USTONS


def _get_hccs_result(hccs_data: list, trace_parser: any) -> list:
    delta_dev = InfoConfReader().get_delta_time()
    _trace = []
    json_data = [InfoConfReader().get_json_pid_data(),
                 InfoConfReader().get_json_tid_data()]
    _result = TraceViewManager.metadata_event(
        [["process_name", json_data[0], json_data[1], trace_parser.scope]])
    for _data_item in hccs_data:
        _trace.extend(
            [['Tx', int(float(_data_item[0]) + delta_dev * NumberConstant.NANO_SECOND) / NumberConstant.USTONS,
              json_data[0], json_data[1],
              OrderedDict([('Tx(MB/s)', _data_item[1])])],
             ['Rx', int(float(_data_item[0]) + delta_dev * NumberConstant.NANO_SECOND) / NumberConstant.USTONS,
              json_data[0], json_data[1],
              OrderedDict([('Rx(MB/s)', _data_item[2])])]]
        )
    _result.extend(TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, _trace))
    return _result


def _get_hcc_events_data(start: int, end: int, devid: str, curs: any) -> list:
    _data = []
    if abs(end - 0) <= NumberConstant.FLT_EPSILON:
        sql = "SELECT timestamp, txThroughput, rxThroughput FROM {0} WHERE device_id " \
              "IS ?".format(DBNameConstant.TABLE_HCCS_EVENTS)
        _data = DBManager.fetch_all_data(curs, sql, (devid,))
    else:
        sql = "SELECT timestamp, txThroughput, rxThroughput FROM {0} WHERE device_id " \
              "IS ? AND timestamp between ? and ?".format(DBNameConstant.TABLE_HCCS_EVENTS)
        _data = DBManager.fetch_all_data(curs, sql, (devid, start, end))
    return _data


def get_hccs_timeline(result_dir: str, devid: str, start: int, end: int) -> str:
    """
    Report hccs timeline
    """
    conn, curs = DBManager.check_connect_db(result_dir, DBNameConstant.DB_HCCS)
    if not conn or not curs:
        return json.dumps({"status": NumberConstant.ERROR, "info": "Failed to connect hccs.db."})
    _data = _get_hcc_events_data(start, end, devid, curs)
    trace_parser = TraceViewer('HCCS')
    _result = _get_hccs_result(_data, trace_parser)
    DBManager.destroy_db_connect(conn, curs)
    return trace_parser.format_trace_events(_result)


def _get_pcie_data(pcie_data: list, trace_parser: any) -> list:
    delta_dev = InfoConfReader().get_delta_time()
    pid = InfoConfReader().get_json_pid_data()
    tid = InfoConfReader().get_json_tid_data()
    trace_data = []
    meta_data = [["process_name", pid, tid, trace_parser.scope]]
    _trace = TraceViewManager.metadata_event(meta_data)
    for data_item in pcie_data:
        if len(data_item) >= 23:
            # check whether the tmp_data has at least 23 domains.
            trace_data.append(trace_parser.format_trace_data(
                data_item, "PCIe_post",
                int(data_item[0] + delta_dev * NumberConstant.NANO_SECOND) / NumberConstant.USTONS,
                pid,
                tid))
            trace_data.append(trace_parser.format_trace_data(
                data_item, "PCIe_nonpost",
                int(data_item[0] + delta_dev * NumberConstant.NANO_SECOND) / NumberConstant.USTONS,
                pid,
                tid))
            trace_data.append(trace_parser.format_trace_data(
                data_item, "PCIe_cpl",
                int(data_item[0] + delta_dev * NumberConstant.NANO_SECOND) / NumberConstant.USTONS,
                pid,
                tid))
            trace_data.append(trace_parser.format_trace_data(
                data_item, "PCIe_nonpost_latency",
                int(data_item[0] + delta_dev * NumberConstant.NANO_SECOND) / NumberConstant.USTONS,
                pid,
                tid))

    _trace += TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, trace_data)
    return _trace


def get_pcie_timeline(param: dict) -> str:
    """
    Report PCIe timeline
    """
    conn, curs = DBManager.check_connect_db(param.get("project_path"), DBNameConstant.DB_PCIE)
    if not (conn and curs):
        return json.dumps({"info": "Failed to connect pcie database. "})
    sql = "select * from {} where device_id=? and timestamp between ? " \
          "and ? and tx_p_bandwidth_max >= tx_p_bandwidth_min;".format(DBNameConstant.TABLE_PCIE)
    data = DBManager.fetch_all_data(curs, sql, (param['device_id'],
                                                param['start_time'],
                                                param['end_time']))
    trace_parser = TraceViewer('PCIe')
    _trace = _get_pcie_data(data, trace_parser)
    DBManager.destroy_db_connect(conn, curs)
    return trace_parser.format_trace_events(_trace)


def get_dvpp_timeline(param: dict) -> str:
    """
    function that provides dvpp data searched from peripheral.db
    :param param: get dvpp timeline params
    :return: json format data
    """
    conn, curs = DBManager.check_connect_db(param['project_path'], 'peripheral.db')
    _result = []
    if not (conn and curs):
        return json.dumps({'status': NumberConstant.ERROR, "info": "The db doesn't exist!"})
    try:
        if not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_DVPP_ORIGIN):
            return json.dumps({'status': NumberConstant.ERROR, "info": "The table doesn't exist."})
    except sqlite3.Error:
        return json.dumps({"status": NumberConstant.ERROR, "info": "No data is collected."})
    else:
        dict_engine_id = get_dvpp_engine_id(Constant.DVPP_TYPE_NAME, conn)
        dvpp_list = get_dvpp_ids(conn) if "all" in param['dvppid'] else param['dvppid']
        _process_dvpp_timeline_data(_result, conn, dvpp_list, dict_engine_id, param)
        return TraceViewer.format_trace_events(_result)
    finally:
        DBManager.destroy_db_connect(conn, curs)


def _process_dvpp_timeline_data(result: list, conn: any, dvpp_list: list, engine_ids: dict, params: dict) -> None:
    delta_dev = InfoConfReader().get_delta_time()
    sort_index = 0
    for dvpp_id in dvpp_list:
        for _type in list(engine_ids.keys()):
            trace_parser = TraceViewer('DVPP/{}/{}'.format(dvpp_id, _type))
            result += TraceViewManager.metadata_event(
                [["process_name", sort_index, InfoConfReader().get_json_tid_data(),
                  trace_parser.scope],
                 ["process_sort_index", sort_index, InfoConfReader().get_json_tid_data(),
                  sort_index]])
            for _id in engine_ids[_type]:
                slice_ = {}
                _dump_data = {}
                _param = {'project_path': params['project_path'],
                          'start_time': params['start_time'],
                          'end_time': params['end_time'],
                          'device_id': params['device_id'],
                          'dvppid': dvpp_id,
                          'engine_type': Constant.DVPP_TYPE_NAME.index(_type),
                          'engine_id': _id}
                slice_['_slice_time'], slice_['_slice_util'] = \
                    get_dvpp_total_data(_param, conn)
                _dump_data["slice_data"] = \
                    {"Engine_{}/frame".format(_id): slice_.get('_slice_time', 0),
                     "Engine_{}/utilization".format(_id): slice_.get('_slice_util', 0)}
                _dump_data["legend"] = \
                    {"Engine_{}/frame".format(_id): get_dvpp_legend()[0],
                     "Engine_{}/utilization".format(_id): get_dvpp_legend()[1]}
                result.extend(trace_parser.multiple_name_dump(_dump_data.get("slice_data", {}),
                                                              _dump_data.get("legend", ""),
                                                              delta_dev,
                                                              sort_index,
                                                              InfoConfReader().get_json_tid_data()))
            sort_index += 1


def _get_network_data(table: dict, sql_: dict, curs: any, *param: any) -> dict:
    dev_id, start, end = param
    dump_data = OrderedDict([("results", {}),
                             ("legends", {})])
    for _func_id in table.get('_func_list', []):
        for _direct in ['tx', 'rx']:
            sql_["_events"] = '{0}_bandwidth_efficiency,{0}_packets,' \
                              '{0}_error_rate,{0}_dropped_rate'.format(_direct)
            sql_['sentence'] = "SELECT timestamp, {1} FROM {0} WHERE device_id " \
                               "IS ? AND func_id IS ? AND timestamp " \
                               "between ? and ?" \
                .format(table.get('_table', ''), sql_.get("_events", ''))
            dump_data.get("results", {})['Port {}/{}'.format(_func_id[0], _direct.capitalize())] = \
                DBManager.fetch_all_data(curs, sql_.get('sentence', ''), (dev_id, _func_id[0], start, end))
            dump_data.get("legends", {})['Port {}/{}'.format(_func_id[0], _direct.capitalize())] = \
                Utils.generator_to_list(item.replace("_", " ").title() for item in sql_.get("_events", "").split(','))
    return dump_data


def get_network_timeline(result_dir: str, devid: str, start: int, end: int, collect_type: str) -> str:
    """
    Return trace-viewer json format RoCE/NIC timeline
    """
    table = {}
    if collect_type == 'roce':
        table['_db'] = DBNameConstant.DB_ROCE_RECEIVE
        table['_table'] = DBNameConstant.TABLE_ROCE_RECEIVE
        table['_header'] = 'RoCE'
    elif collect_type == 'nic':
        table['_db'] = DBNameConstant.DB_NIC_RECEIVE
        table['_table'] = DBNameConstant.TABLE_NIC_RECEIVE
        table['_header'] = 'NIC'
    else:
        return json.dumps(
            {"status": NumberConstant.ERROR, "info": "Failed to get collection type."})
    conn, curs = DBManager.check_connect_db(result_dir, table.get('_db', ""))
    if not conn or not curs:
        return json.dumps(
            {"status": NumberConstant.ERROR, "info": "Failed to connect {0}.".format(table.get('_db'))})

    table['delta_dev'] = InfoConfReader().get_delta_time()
    sql_ = {'sentence': "SELECT DISTINCT func_id FROM {} " \
                        "WHERE device_id IS ?".format(table.get('_table', ''))}
    table['_func_list'] = DBManager.fetch_all_data(curs, sql_.get('sentence', ''), (devid,))
    trace_parser = TraceViewer(table.get('_header', ''))
    _result = []
    _result += TraceViewManager.metadata_event(
        [["process_name", InfoConfReader().get_json_pid_data(),
          InfoConfReader().get_json_tid_data(), trace_parser.scope]])
    dump_data = _get_network_data(table, sql_, curs, devid, start, end)
    _result += trace_parser.multiple_name_dump(dump_data.get("results", []),
                                               dump_data.get("legends", ""), table.get('delta_dev', 0),
                                               InfoConfReader().get_json_pid_data(),
                                               InfoConfReader().get_json_tid_data())
    DBManager.destroy_db_connect(conn, curs)
    return trace_parser.format_trace_events(_result)


def _get_aicore_utilization_data(aicore_result: dict, pid: str, tid: str) -> list:
    trace_data = []
    for aicore_name in aicore_result.get('data', {}).get('usage', {}):
        for result_value in aicore_result.get('data', {}).get('usage', {}).get(aicore_name, ''):
            if aicore_name == 'average':
                trace_name = 'Average'
            else:
                trace_name = 'Core {}'.format(aicore_name)
            trace_data.append(
                (trace_name, round(float(result_value[0]) * NumberConstant.MS_TIME_RATE, CommonConstant.ROUND_SIX),
                 pid,
                 tid,
                 OrderedDict([('Utilization(%)', result_value[1])])))
    return trace_data


def get_aicore_utilization_timeline(param: dict) -> str:
    """
    get ai core utilization trace view data
    """
    try:
        aicore_result = get_aicore_utilization(param.get('project_path', ''), param.get('device_id', 0),
                                               NumberConstant.DEFAULT_NUMBER, NumberConstant.DEFAULT_START_TIME,
                                               NumberConstant.DEFAULT_END_TIME)
    except sqlite3.Error as error:
        logging.error(str(error), exc_info=Constant.TRACE_BACK_SWITCH)
        return json.dumps({"status": NumberConstant.ERROR, "info": "Failed to get ai core utilization data."})

    if len(aicore_result) >= NumberConstant.MAX_STR_LENGTH:
        warn(os.path.basename(__file__),
             "The size of AI Core utilization data exceeds 8 MB. "
             "Parsing the data may take several minutes..")

    aicore_result = JsonManager.loads(aicore_result)
    if not aicore_result:
        return json.dumps({"status": NumberConstant.ERROR, "info": "Failed to get ai core utilization data."})

    pid = InfoConfReader().get_json_pid_data()
    tid = InfoConfReader().get_json_tid_data()

    meta_data = [["process_name", pid, tid, TraceViewHeaderConstant.PROCESS_AI_CORE_UTILIZATION]]
    result_data = TraceViewManager.metadata_event(meta_data)
    try:
        trace_data = _get_aicore_utilization_data(aicore_result, pid, tid)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as error:
        logging.error(str(error), exc_info=Constant.TRACE_BACK_SWITCH)
        return json.dumps({"status": NumberConstant.ERROR, "info": "Failed to get ai core utilization data."})
    result_data += TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, trace_data)
    return TraceViewer.format_trace_events(result_data)


def _get_acl_data(acl_iter_datas: list) -> list:
    pid_values = set()
    tid_values = set()
    for acl_iter_data in acl_iter_datas:
        pid_values.add(acl_iter_data[3])
        tid_values.add(acl_iter_data[4])
    meta_data = Utils.generator_to_list(["process_name", pid_value,
                                         InfoConfReader().get_json_tid_data(), TraceViewHeaderConstant.PROCESS_ACL]
                                        for pid_value in pid_values)
    meta_data.extend(Utils.generator_to_list(["thread_name", pid_value, tid_value,
                                              "Thread {}".format(tid_value)] for tid_value in tid_values
                                             for pid_value in pid_values))
    meta_data.extend(Utils.generator_to_list(["thread_sort_index", pid_value, tid_value,
                                              tid_value] for tid_value in tid_values
                                             for pid_value in pid_values))
    result_data = TraceViewManager.metadata_event(meta_data)

    return result_data


def _format_single_data(top_down_datas: list) -> list:
    """
    conforms acl and ge data to the trace view format
    """
    trace_data = []
    for top_down_data in top_down_datas:
        if top_down_data[0] == TraceViewHeaderConstant.PROCESS_ACL:
            trace_data_args = OrderedDict([("Mode", top_down_data[6]),
                                           ("Process Id", top_down_data[4]),
                                           ("Thread Id", top_down_data[5])
                                           ])
        else:
            trace_data_args = OrderedDict([("Model Name", top_down_data[6]),
                                           ("Model Id", top_down_data[7])
                                           ])
        trace_data_pice = [top_down_data[1], top_down_data[4], top_down_data[5],
                           int(top_down_data[2]) / NumberConstant.CONVERSION_TIME,
                           int(top_down_data[3]) / NumberConstant.CONVERSION_TIME,
                           trace_data_args]
        trace_data.append(trace_data_pice)
    return trace_data


def _get_ge_data(ge_datas: list, pid: int) -> list:
    _ge_trace_data = []
    for ge_data in ge_datas:
        ge_trace_data = [(TraceViewHeaderConstant.PROCESS_GE, "Input", ge_data[0], ge_data[1],
                          pid,
                          ge_data[8], ge_data[6], ge_data[7]),
                         (TraceViewHeaderConstant.PROCESS_GE, "Infer", ge_data[2], ge_data[3],
                          pid,
                          ge_data[8], ge_data[6], ge_data[7]),
                         (TraceViewHeaderConstant.PROCESS_GE, "Output", ge_data[4], ge_data[5],
                          pid,
                          ge_data[8], ge_data[6], ge_data[7])]
        ge_trace_data = _format_single_data(ge_trace_data)
        _ge_trace_data.extend(ge_trace_data)
    return _ge_trace_data


def _get_ge_result_data(ge_datas: list, pid: int, tid: int) -> list:
    result_data = []
    tid_values = set()
    for ge_data in ge_datas:
        tid_values.add(ge_data[8])
    meta_data = [["process_name", pid, tid, TraceViewHeaderConstant.PROCESS_GE]]
    meta_data.extend(["thread_name", InfoConfReader().get_json_pid_data(),
                      tid_value, "Thread {}".format(tid_value)]
                     for tid_value in tid_values)
    meta_data.extend(["thread_sort_index",
                      pid,
                      tid_value, tid_value] for tid_value in tid_values)
    result_data.extend(TraceViewManager.metadata_event(meta_data))
    return result_data


def _get_ge_sql() -> str:
    sql = "select input_start, (input_end-input_start) as input_duration,infer_start, " \
          "(infer_end-infer_start) as infer_duration, output_start,(output_end-output_start)" \
          " as output_duration, model_name, model_id, thread_id as model_duration from {}" \
        .format(DBNameConstant.TABLE_GE_MODEL_TIME)
    return sql


def get_ge_timeline_data(project_path: str) -> str:
    """
    get ge trace view data
    """
    logging.info("Start to get ge tracing data.")
    conn, cur = DBManager.check_connect_db(project_path, DBNameConstant.DB_GE_MODEL_TIME)
    try:
        if conn and cur:
            if not DBManager.judge_table_exist(cur, DBNameConstant.TABLE_GE_MODEL_TIME):
                return json.dumps({"status": NumberConstant.ERROR,
                                   "info": "table {0} does not exist.".format(DBNameConstant.TABLE_GE_MODEL_TIME)})
            ge_datas = DBManager.fetch_all_data(cur, _get_ge_sql())
            if ge_datas:
                pid = InfoConfReader().get_json_pid_data()
                tid = InfoConfReader().get_json_tid_data()
                result_data = _get_ge_result_data(ge_datas, pid, tid)
                _ge_trace_data = _get_ge_data(ge_datas, pid)
                result_data.extend(
                    TraceViewManager.time_graph_trace(TraceViewHeaderConstant.TOP_DOWN_TIME_GRAPH_HEAD, _ge_trace_data))
                return json.dumps(result_data)
        return json.dumps(
            {"status": NumberConstant.ERROR, "info": "Failed to connect {0}.".format(DBNameConstant.DB_GE_MODEL_INFO)})
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as error:
        logging.error(str(error), exc_info=Constant.TRACE_BACK_SWITCH)
        return json.dumps(
            {"status": NumberConstant.ERROR, "info": "Failed to get ge data."})
    finally:
        DBManager.destroy_db_connect(conn, cur)
