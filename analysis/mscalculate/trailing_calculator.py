"""
This script is used to calculate cluster scene data smearing time.
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
from analyzer.scene_base.profiling_scene import ProfilingScene
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
    SLOW_TAG = 1
    QUICK_TAG = 0

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

    def run(self: any) -> list:
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
        slow_node = []
        if not self.trailing_dict:
            return slow_node
        for value in self.trailing_dict.values():
            sum_time += value
        avg_bound = sum_time / len(self.trailing_dict.values())
        for key, value in self.trailing_dict.items():
            if (value - avg_bound)/avg_bound > 0.2:
                slow_node.append([key])
        return slow_node

    def calculate(self: any, data_path) -> None:
        """
        get and update data
        :return: None
        """
        time = self.get_step_data(data_path)
        if time:
            self.trailing_dict[data_path] = time[0][0]
