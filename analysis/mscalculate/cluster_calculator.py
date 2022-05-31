"""
This script is used to calculate cluster scene data smearing time.
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.cluster_tuning import ClusterTuning
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

    def __init__(self: any):
        self._cluster_list = ClusterTuning().cluster_list

    def ms_run(self: any) -> None:
        """
        entrance for calculating op_summary
        :return: None
        """
        for data_path in self._cluster_list:
            ProfilingScene().init(data_path)
            if not ProfilingScene().is_step_trace():
                continue
            self.calculate(data_path)
        self.calculate_slow_node()

    def calculate_slow_node(self: any) -> None:
        sum_time = 0
        cluster_data_time_list = ClusterTuning().avg_time_dict
        if not cluster_data_time_list:
            return
        for value in cluster_data_time_list.values():
            sum_time += value[0]
        avg_bound = sum_time / len(cluster_data_time_list.values())
        for key, value in cluster_data_time_list.items():
            if (value[0] - avg_bound)/avg_bound > 0.2:
                ClusterTuning().avg_time_dict[key].append(self.SLOW_TAG)
            else:
                ClusterTuning().avg_time_dict[key].append(self.QUICK_TAG)

    def calculate(self: any, data_path) -> None:
        """
        get and update data
        :return: None
        """
        time = self.get_step_data(data_path)
        if time:
            ClusterTuning().set_node_avg_time(data_path, time[0][0])

    @staticmethod
    def get_step_data(data_path: str) -> list:
        sql_path = PathManager.get_db_path(data_path, DBNameConstant.DB_TRACE)
        conn, curs = DBManager.check_connect_db_path(sql_path)
        avg_time = []
        if DBManager.check_tables_in_db(sql_path, 'training_trace'):
            avg_time = DBManager.fetch_all_data(curs, "select avg(data_aug_bound) from training_trace")
        return avg_time

    def save(self: any) -> None:
        """
        save data into database
        :return: None
        """
        self.calculate()
