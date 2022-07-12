"""
This script is used to load training_trace data from db
Copyright Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
"""

import os
import json
import logging
import sqlite3
from collections import OrderedDict
from functools import wraps

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.empty_class import EmptyClass
from common_func.info_conf_reader import InfoConfReader
from common_func.msvp_common import is_number
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_constant import MsvpConstant
from common_func.path_manager import PathManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.utils import Utils
from common_func.step_trace_constant import StepTraceConstant
from saver.training.training_trace_saver import TrainingTraceSaver


def catch_exception(fun: any) -> any:
    """
    catch exception
    :param fun: function
    :return: function
    """

    @wraps(fun)
    def wrapper(trace_data: list) -> any:
        """
        wrapper function
        :param trace_data: trace data
        :return: any
        """
        try:
            res = fun(trace_data)
            return res
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            res = None
            return res
        finally:
            res = None

    return wrapper


class TimeLineJsonMaker:
    """
    make training trace timeline json
    """

    @staticmethod
    def create_trace_parm(trace_parm: dict, data: list) -> None:
        """
        create trace parm
        :param trace_parm: trace parm
        :param data: data
        :return: void
        """
        trace_parm[StepTraceConstant.ITER_ID] = data[0]
        trace_parm[StepTraceConstant.FORWARD_PROPAGATION] = data[1]
        trace_parm[StepTraceConstant.BACK_PROPAGATION] = data[2]
        trace_parm[StepTraceConstant.STEP_END] = data[3]
        trace_parm[StepTraceConstant.ITER_TIME] = data[4]
        trace_parm[StepTraceConstant.FORWARD_TO_BACK] = data[5]
        trace_parm[StepTraceConstant.ITERATION_REFRESH] = data[6]
        trace_parm[StepTraceConstant.DATA_AUGMENTATION] = data[7] if is_number(data[7]) else 0

    @staticmethod
    def make_iter_time(trace_parm: dict, pid: int, tid: int) -> list:
        """
        make iter time
        :param trace_parm: trace parm
        :param pid: pid
        :param tid: tid
        :return: list
        """
        return ["Iteration {}".format(trace_parm[StepTraceConstant.ITER_ID]),
                pid, tid,
                trace_parm.get(StepTraceConstant.STEP_END) - trace_parm.get(StepTraceConstant.ITER_TIME),
                trace_parm.get(StepTraceConstant.ITER_TIME),
                OrderedDict([("Iteration ID", trace_parm[StepTraceConstant.ITER_ID]),
                             ("FP Start", trace_parm[StepTraceConstant.FORWARD_PROPAGATION]),
                             ("BP End", trace_parm[StepTraceConstant.BACK_PROPAGATION]),
                             ("Iteration End", trace_parm[StepTraceConstant.STEP_END]),
                             ("Iteration Time(ns)",
                              round(trace_parm.get(StepTraceConstant.ITER_TIME) * NumberConstant.NS_TO_US,
                                    NumberConstant.ROUND_TWO_DECIMAL))]),
                "Iteration Time"]

    @staticmethod
    def make_fp_bp_data(trace_parm: dict, pid: int, tid: int) -> list:
        """
        make fp bp data
        :param trace_parm: data parm
        :param pid: pid
        :param tid: tid
        :return: list
        """
        return ["FP_BP Time {}".format(trace_parm[StepTraceConstant.ITER_ID]),
                pid, tid,
                trace_parm[StepTraceConstant.FORWARD_PROPAGATION],
                (trace_parm[StepTraceConstant.BACK_PROPAGATION] - trace_parm[StepTraceConstant.FORWARD_PROPAGATION]),
                OrderedDict([("Iteration ID", trace_parm[StepTraceConstant.ITER_ID]),
                             ("FP Start", trace_parm[StepTraceConstant.FORWARD_PROPAGATION]),
                             ("BP End", trace_parm[StepTraceConstant.BACK_PROPAGATION]),
                             ("FP_BP Time(ns)",
                              round(trace_parm.get(StepTraceConstant.FORWARD_TO_BACK) * NumberConstant.NS_TO_US,
                                    NumberConstant.ROUND_TWO_DECIMAL))]),
                "FP_BP Time"]

    @staticmethod
    def make_grad_refresh_data(trace_parm: dict, pid: int, tid: int) -> list:
        """
        make grad refresh data
        :param trace_parm: trace parm
        :param pid: pid
        :param tid: tid
        :return: list
        """
        return ["Iteration Refresh {}".format(trace_parm[StepTraceConstant.ITER_ID]),
                pid, tid,
                trace_parm[StepTraceConstant.BACK_PROPAGATION],
                (trace_parm[StepTraceConstant.STEP_END] - trace_parm[StepTraceConstant.BACK_PROPAGATION]),
                OrderedDict([("Iteration ID", trace_parm[StepTraceConstant.ITER_ID]),
                             ("BP End", trace_parm[StepTraceConstant.BACK_PROPAGATION]),
                             ("Iteration End", trace_parm[StepTraceConstant.STEP_END]),
                             ("Iteration Refresh(ns)",
                              round(trace_parm.get(StepTraceConstant.ITERATION_REFRESH) * NumberConstant.NS_TO_US,
                                    NumberConstant.ROUND_TWO_DECIMAL))]),
                "Iteration Refresh"]

    @staticmethod
    def make_data_aug_dict0(trace_parm: dict, pid: int, tid: int) -> dict:
        """
        make data aug dict0
        :param trace_parm: trace parm
        :param pid: pid
        :param tid: tid
        :return: dict
        """
        return OrderedDict([("name", "Data_aug Bound {}".format(trace_parm[StepTraceConstant.ITER_ID])),
                            ("cat", "Data_aug Bound"),
                            ("ph", "s"),
                            ("ts", trace_parm[StepTraceConstant.STEP_END]),
                            ("pid", pid),
                            ("tid", tid),
                            ("id", "Data_aug Bound {}".format(trace_parm[StepTraceConstant.ITER_ID])),
                            ("args", OrderedDict([("Iteration ID", trace_parm[StepTraceConstant.ITER_ID])]))])

    @staticmethod
    def make_data_aug_dict1(trace_parm: dict, pid: int, tid: int) -> dict:
        """
        make data aug dict1
        :param trace_parm: trace parm
        :param pid: pid
        :param tid: tid
        :return: dict
        """
        data_aug_bound = round(trace_parm[StepTraceConstant.DATA_AUGMENTATION],
                               NumberConstant.ROUND_TWO_DECIMAL)
        return OrderedDict([("name", "Data_aug Bound {}".format(trace_parm[StepTraceConstant.ITER_ID])),
                            ("cat", "Data_aug Bound"),
                            ("ph", "t"),
                            ("ts", (trace_parm[StepTraceConstant.STEP_END] + trace_parm[
                                StepTraceConstant.DATA_AUGMENTATION])),
                            ("pid", pid),
                            ("tid", tid),
                            ("id", "Data_aug Bound {}".format(trace_parm[StepTraceConstant.ITER_ID])),
                            ("args", OrderedDict([("Data_aug Bound(us)",
                                                   data_aug_bound)]))])


class StepTraceViewer:
    """
    viewer of training trace data
    """
    model_to_pid = {}

    @staticmethod
    def get_step_trace_data(curs: any, message: dict) -> list:
        """
        get training trace data
        :param cur: sqlite cur
        :param message: message
        :return: data
        """
        sql = "select iteration_id, " \
              "(case when FP_start={2} then 'N/A' else FP_start end), " \
              "(case when BP_end={2} then 'N/A' else BP_end end), " \
              "iteration_end, " \
              "(case when iteration_time={2} then 'N/A' else iteration_time*{0} end), " \
              "(case when fp_bp_time={2} then 'N/A' else fp_bp_time*{0} end), " \
              "(case when grad_refresh_bound={2} then 'N/A' else grad_refresh_bound*{0} end), " \
              "(case when data_aug_bound={2} then 'N/A' else data_aug_bound*{0} end), " \
              "(case when model_id={3} then 'N/A' else model_id end) " \
              " from {1} where device_id=?".format(
            StepTraceConstant.syscnt_to_micro(),
            DBNameConstant.TABLE_TRAINING_TRACE,
            NumberConstant.NULL_NUMBER,
            NumberConstant.DEFAULT_MODEL_ID)
        data = DBManager.fetch_all_data(curs, sql, (message["device_id"],))
        return data

    @staticmethod
    def query_top_total(message: any) -> dict:
        """
        Rewrite gRPC QueryTopTotal method.
        Return a QueryTopTotalResponse protobuf message for client's request
        defined by QueryTopTotalResponse.
        """
        if not message:
            logging.error("query_top_total GRPC message empty")
            return {}
        resp = {}
        sql_path = message["sql_path"]
        try:
            conn = TrainingTraceSaver.create_trace_conn(sql_path)
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return resp
        try:
            values = StepTraceViewer.__count_trace(conn)
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return resp
        resp["total_count"] = values
        return resp

    @staticmethod
    def get_model_pid(data: list) -> int:
        """
        get model pid
        :param data: data
        :return: pid
        """
        return StepTraceViewer.model_to_pid.get(data[8])

    @staticmethod
    def make_model_meta(result_data: list, trace_data: list) -> None:
        """
        make model meta
        :param result_data: result data
        :param trace_data: trace data
        :return: void
        """
        if not trace_data:
            return
        if trace_data[0][8] == "N/A":
            model_ids = ["N/A"]
        else:
            model_ids_set = set(map(lambda trace_datum: trace_datum[8], trace_data))
            model_ids = sorted(model_ids_set)
        result_data.extend(TraceViewManager.metadata_event(
            [["process_name", InfoConfReader().get_json_pid_data(),
              InfoConfReader().get_json_tid_data(), "Step Trace"]]))
        for i, model_id in enumerate(model_ids):
            StepTraceViewer.model_to_pid[model_id] = InfoConfReader().get_json_pid_data() + i
            result_data.extend(TraceViewManager.metadata_event(
                [["thread_name", InfoConfReader().get_json_pid_data(),
                  StepTraceViewer.model_to_pid.get(model_id), "Model ID:{}".format(model_id)],
                 ["thread_sort_index", InfoConfReader().get_json_pid_data(),
                  StepTraceViewer.model_to_pid.get(model_id), model_id]
                 ]))

    @staticmethod
    def add_reduce_headers(conn: any, headers: list, message: dict) -> None:
        """
        add reduece headers
        :param conn: sqlite connection
        :param headers: headers
        :param message: message
        :return:
        """
        if DBManager.judge_table_exist(conn.cursor(), DBNameConstant.TABLE_ALL_REDUCE):
            reduce_data = conn.cursor().execute(
                "select count(*) from  {0} where device_id=?"
                " group by iteration_end, model_id;".format(DBNameConstant.TABLE_ALL_REDUCE),
                (message["device_id"],)).fetchone()
            if reduce_data:
                headers += ["Reduce Start", "Reduce Duration(us)"] * \
                           int(reduce_data[0])

    @staticmethod
    def format_reduce_json(data: list, trace_parm: dict, pid: int, tid: int, result_data: list) -> None:
        """
        format reduce json
        :param data: data
        :param trace_parm: trace parm
        :param pid: pid
        :param tid: index
        :param result_data: result data
        :return: void
        """
        reduce_index = 9
        if len(data) > reduce_index:
            i = 0
            for reduce_data in data[reduce_index:]:
                reduce_trace_data = []
                trace_parm[StepTraceConstant.REDUCE_START] = reduce_data[0]
                trace_parm[StepTraceConstant.REDUCE_END] = reduce_data[1]
                grad_refresh_data = \
                    ["Reduce_{}_{}".format(trace_parm[StepTraceConstant.ITER_ID], i),
                     pid, tid,
                     trace_parm[StepTraceConstant.REDUCE_START],
                     (trace_parm[StepTraceConstant.REDUCE_END] -
                      trace_parm[StepTraceConstant.REDUCE_START]),
                     OrderedDict([
                         ("Iteration ID", trace_parm[StepTraceConstant.ITER_ID]),
                         ("Reduce Start {}".format(i),
                          trace_parm[StepTraceConstant.REDUCE_START]),
                         ("Reduce End {}".format(i),
                          trace_parm[StepTraceConstant.REDUCE_END])]),
                     "Reduce"]
                reduce_trace_data.append(grad_refresh_data)
                result_data.extend(TraceViewManager.time_graph_trace(
                    TraceViewHeaderConstant.GRPC_TIME_GRAPH_HEAD, reduce_trace_data))
                i = i + 1

    @staticmethod
    def get_step_trace_summary(message: dict) -> tuple:
        """
        @param message
        Rewrite gRPC get_training_trace method.
        Return a GetTraceResponse protobuf message for client's request
        defined by GetTraceRequest.
        """
        headers = []

        sql_path = PathManager.get_sql_dir(message.get("project_path"))
        conn_path = os.path.join(sql_path, DBNameConstant.DB_TRACE)

        if not DBManager.check_tables_in_db(conn_path, DBNameConstant.TABLE_TRAINING_TRACE):
            return MsvpConstant.MSVP_EMPTY_DATA
        conn, curs = DBManager.check_connect_db_path(conn_path)
        if not (conn and curs):
            return MsvpConstant.MSVP_EMPTY_DATA
        data = StepTraceViewer.get_step_trace_data(curs, message)
        headers.append("Model ID")
        try:
            StepTraceViewer.add_reduce_headers(conn, headers, message)
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return MsvpConstant.MSVP_EMPTY_DATA
        merge_data = StepTraceViewer._reformat_step_trace_data(data, conn)
        DBManager.destroy_db_connect(conn, curs)
        return headers, merge_data, len(merge_data)

    @staticmethod
    def get_step_trace_timeline(message: dict) -> str:
        """
        @param message
        Rewrite gRPC get_training_trace method.
        Return a GetTraceResponse protobuf message for client's request
        defined by GetTraceRequest.
        """
        sql_path = PathManager.get_sql_dir(message.get("project_path"))
        conn_path = os.path.join(sql_path, DBNameConstant.DB_TRACE)

        if not DBManager.check_tables_in_db(conn_path, DBNameConstant.TABLE_TRAINING_TRACE):
            return json.dumps(MsvpConstant.EMPTY_LIST)

        conn, curs = DBManager.check_connect_db_path(conn_path)
        data = StepTraceViewer.get_step_trace_data(curs, message)
        curs.close()

        return StepTraceViewer.get_trace_timeline_data(conn, data)

    @staticmethod
    def get_one_iter_timeline_data(result_dir: str, index_id: int, model_id: int) -> str:
        """
        get one iteration timeline data
        :param result_dir: data dir
        :param index_id: step trace id
        :param model_id: model id
        :return: timeline json data
        """
        sql_path = PathManager.get_sql_dir(result_dir)
        conn_path = os.path.join(sql_path, DBNameConstant.DB_TRACE)

        if not DBManager.check_tables_in_db(conn_path, DBNameConstant.TABLE_TRAINING_TRACE):
            return json.dumps(MsvpConstant.EMPTY_LIST)

        conn, curs = DBManager.check_connect_db_path(conn_path)
        values = StepTraceViewer.__select_trace_one_iter(
            curs, index_id, model_id)

        return StepTraceViewer.get_trace_timeline_data(conn, values)

    @staticmethod
    def transfer_trace_unit(trace: list) -> None:
        """
        transfer trace unit
        :param trace: trace
        :return: void
        """
        for i in range(1, 4):
            if trace[i] != "N/A":
                trace[i] = InfoConfReader().time_from_syscnt(trace[i], NumberConstant.MICRO_SECOND)

    @staticmethod
    def get_trace_timeline_data(cnn: any, values: list) -> any:
        """
        get trace timeline data
        :param cnn: db conn
        :param values: training data
        :return:json format data
        """
        step = 0
        data = list(range(len(values)))
        try:
            for line in values:
                trace = list(line)

                all_reduce = StepTraceViewer.__select_reduce(cnn,
                                                             trace)
                all_reduce = Utils.generator_to_list(
                    list(map(lambda cnt:
                             InfoConfReader().time_from_syscnt(cnt, NumberConstant.MICRO_SECOND),
                             data)) for data in all_reduce)
                trace.extend(all_reduce)
                StepTraceViewer.transfer_trace_unit(trace)
                data[step] = tuple(trace)
                # Cursor step moved 1 step
                step = step + 1
            return StepTraceViewer.__format_trace_json(data)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) \
                as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            if cnn:
                DBManager.destroy_db_connect(cnn, cnn.cursor())
        return EmptyClass("no trace timeline data")

    @staticmethod
    def _reformat_step_trace_data(data: list, conn: any) -> list:
        merge_data = []
        for line in data:
            trace = list(line)
            if len(trace) >= 4:  # trace[3] refers to iteration end
                reduce_data = StepTraceViewer.__select_reduce(conn,
                                                              trace)
                for item in reduce_data:
                    trace += [item[0], (item[1] - item[0]) * StepTraceConstant.syscnt_to_micro()]
            merge_data.append(trace)
        return merge_data

    @staticmethod
    @catch_exception
    def __format_trace_json(trace_data: list) -> any:
        """
        Parse hwts protobuf message to json format
        trace_parm//100: Convert 10ns-level timestamp to us-level timestamp
        tid=0:Iteration Time
        tid=1:FP_BP Time;Grad_refresh Bound
        tid=2:Reduce time
        """
        result_data = []
        trace_parm = {}
        result_dict = {}
        StepTraceViewer.make_model_meta(result_data, trace_data)
        pid = InfoConfReader().get_json_pid_data()
        for _, data in enumerate(trace_data):
            trace_view_data = []
            TimeLineJsonMaker.create_trace_parm(trace_parm, data)

            tid = StepTraceViewer.get_model_pid(data)

            iter_time_data = TimeLineJsonMaker.make_iter_time(trace_parm, pid, tid)

            if data[1] == "N/A" or data[2] == "N/A":
                trace_view_data.append(iter_time_data)
                result_data.extend(
                    TraceViewManager.time_graph_trace(TraceViewHeaderConstant.GRPC_TIME_GRAPH_HEAD, trace_view_data))

            else:
                fp_bp_data = TimeLineJsonMaker.make_fp_bp_data(trace_parm, pid, tid)
                grad_refresh_data = TimeLineJsonMaker.make_grad_refresh_data(trace_parm, pid, tid)
                result_dict["data_aug_dict0"] = TimeLineJsonMaker.make_data_aug_dict0(trace_parm, pid, tid)
                result_dict["data_aug_dict1"] = TimeLineJsonMaker.make_data_aug_dict1(trace_parm, pid, tid)

            trace_view_data.append(iter_time_data)
                trace_view_data.append(fp_bp_data)
                trace_view_data.append(grad_refresh_data)
                result_data.extend(
                    TraceViewManager.time_graph_trace(TraceViewHeaderConstant.GRPC_TIME_GRAPH_HEAD, trace_view_data))
                result_data.extend([result_dict.get("data_aug_dict0", {}), result_dict.get("data_aug_dict1", {})])

            StepTraceViewer.format_reduce_json(data, trace_parm, pid, tid, result_data)

        return json.dumps(result_data)

    @staticmethod
    def __count_trace(conn: any) -> any:
        """Select date from traing_trace limited by count and sort"""
        curs = conn.cursor()
        sql = "SELECT COUNT(*) FROM {0} group by device_id".format(DBNameConstant.TABLE_TRAINING_TRACE)
        curs.execute(sql)
        result = curs.fetchone()
        if result:
            result = result[0]
        else:
            result = 0
        curs.close()
        return result

    @staticmethod
    def __select_reduce(conn: any, trace: list) -> list:
        """
        Select date from all_reduce table with specific ids.
        :param conn: connect to database
        :param trace: trace data
        :return: result
        """
        curs = conn.cursor()
        iteration_end = trace[3]
        model_id = NumberConstant.DEFAULT_MODEL_ID if trace[-1] == "N/A" else trace[-1]

        sql = "select start, end from {0} " \
              "where iteration_end=? and model_id=?" \
            .format(DBNameConstant.TABLE_ALL_REDUCE)
        result = DBManager.fetch_all_data(curs, sql, (iteration_end, model_id))
        curs.close()
        return result

    @staticmethod
    def __select_trace_one_iter(curs: any, index_id: int, model_id: int) -> list:
        """
        Select date from traing_trace limited by count and sort
        """

        sql = "select iteration_id, " \
              "(case when FP_start={2} then 'N/A' else FP_start end), " \
              "(case when BP_end={2} then 'N/A' else BP_end end), " \
              "iteration_end, " \
              "(case when iteration_time={2} then 'N/A' else iteration_time*{0} end), " \
              "(case when fp_bp_time={2} then 'N/A' else fp_bp_time*{0} end), " \
              "(case when grad_refresh_bound={2} then 'N/A' else grad_refresh_bound*{0} end), " \
              "(case when data_aug_bound={2} then 'N/A' else data_aug_bound*{0} end), " \
              "(case when model_id={3} then 'N/A' else model_id end) " \
              "from {1} where model_id={4} and iteration_id={5}".format(StepTraceConstant.syscnt_to_micro(),
                                                                        DBNameConstant.TABLE_TRAINING_TRACE,
                                                                        NumberConstant.NULL_NUMBER,
                                                                        NumberConstant.DEFAULT_MODEL_ID,
                                                                        model_id,
                                                                        index_id)
        curs.execute(sql)
        result = curs.fetchall()
        # index_id
        curs.close()
        return result
