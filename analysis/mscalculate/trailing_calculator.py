"""
This script is used to calculate cluster scene data smearing time.
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import logging
import os

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.path_manager import PathManager
from common_func.step_trace_constant import StepTraceConstant
from model.stars.op_summary_model import OpSummaryModel
from mscalculate.interface.icalculator import ICalculator


class TrailingCalculator:
    """
    create op op_summary db.
    """
    SLOW_THRESHOLD = 0.2

    def __init__(self: any, path_list: list) -> None:
        self._cluster_list = path_list
        self.trailing_dict = {}

    @staticmethod
    def get_step_data(data_path: str) -> list:
        sql_path = PathManager.get_db_path(data_path, DBNameConstant.DB_TRACE)
        conn, curs = DBManager.check_connect_db_path(sql_path)
        avg_time = []
        if DBManager.check_tables_in_db(sql_path, 'training_trace'):
            avg_time = DBManager.fetch_all_data(curs, "select avg(data_aug_bound) from training_trace")
        return avg_time

    def ms_run(self: any) -> list:
        """
        entrance for calculating op_summary
        :return: None
        """
        for data_path in self._cluster_list:
            ProfilingScene().init(data_path)
            if not ProfilingScene().is_step_trace():
                continue
            self.calculate(data_path)
        return self.calculate_slow_node()

    def calculate_slow_node(self: any) -> list:
        sum_time = 0
        slow_node_list = []
        if not self.trailing_dict:
            return slow_node_list
        for value in self.trailing_dict.values():
            sum_time += value
        if not sum_time:
            return slow_node_list
        try:
            avg_bound = sum_time / len(self.trailing_dict.values())
        except (ZeroDivisionError, TypeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return slow_node_list
        try:
            for key, value in self.trailing_dict.items():
                bias = (value - avg_bound) / avg_bound
                if bias > self.SLOW_THRESHOLD:
                    slow_node = {'slow_node': os.path.basename(os.path.dirname(key)), 'avg_node_cost': avg_bound,
                                 'slow_node_cost': value, 'slow_ratio': bias}
                    slow_node_list.append(slow_node)
        except (ZeroDivisionError, TypeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return slow_node_list
        return slow_node_list

    def calculate(self: any, data_path) -> None:
        """
        get and update data
        :return: None
        """
        time = self.get_step_data(data_path)
        if time:
            self.trailing_dict[data_path] = time[0][0]
