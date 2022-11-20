#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from common_func.constant import Constant
from common_func.db_manager import DBManager, ClassRowType
from common_func.db_name_constant import DBNameConstant
from common_func.ms_multi_process import MsMultiProcess
from msmodel.stars.acc_pmu_model import AccPmuModel
from mscalculate.interface.icalculator import ICalculator
from profiling_bean.db_dto.acc_pmu_dto import AccPmuOriDto
from profiling_bean.prof_enum.data_tag import DataTag


class AccPmuCalculator(ICalculator, MsMultiProcess):
    """
    calculate biu perf data
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.sample_config = sample_config
        self.file_list = file_list
        self.result_dir = self.sample_config.get("result_dir")
        self.model = AccPmuModel(self.result_dir, DBNameConstant.DB_ACC_PMU,
                                 [DBNameConstant.TABLE_ACC_PMU_ORIGIN_DATA])
        self._data = []

    def ms_run(self: any) -> None:
        """
        calculate for biu perf
        :return: None
        """
        if not self.file_list.get(DataTag.STARS_LOG) and not self.file_list.get(DataTag.SOC_PROFILER):
            return
        with AccPmuModel(self.result_dir, DBNameConstant.DB_ACC_PMU,
                         [DBNameConstant.TABLE_ACC_PMU_ORIGIN_DATA]) as self.model:

            if not self.model.check_table():
                return
            self.calculate()
            self.save()

    def calculate(self: any) -> None:
        task_time = self.get_task_time_form_acsq()
        all_data_sql = "select * from {}".format(DBNameConstant.TABLE_ACC_PMU_ORIGIN_DATA)
        ori_data = DBManager.fetch_all_data(self.model.cur, all_data_sql, dto_class=AccPmuOriDto)
        self._data = [(data.task_id, data.stream_id, data.acc_id, data.block_id,
                       data.read_bandwidth, data.write_bandwidth,
                       data.read_ost, data.write_ost, data.timestamp)
                      + task_time.get(data.task_id, (Constant.DEFAULT_VALUE, Constant.DEFAULT_VALUE))
                      for data in ori_data]

    def save(self: any) -> None:
        self.model.drop_table(DBNameConstant.TABLE_ACC_PMU_DATA)
        self.model.table_list = [DBNameConstant.TABLE_ACC_PMU_DATA]
        self.model.create_table()
        self.model.insert_data_to_db(DBNameConstant.TABLE_ACC_PMU_DATA, self._data)

    def get_task_time_form_acsq(self: any) -> dict:
        """
        get task start_time and dur_time
        :return: dict
        """
        task_time_dict = {}
        conn, curs = DBManager.check_connect_db(self.result_dir, DBNameConstant.DB_ACSQ)
        if not (conn and curs):
            return task_time_dict
        sql = 'select task_id, start_time, task_time from {}'.format(DBNameConstant.TABLE_ACSQ_TASK)
        if not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_ACSQ_TASK):
            return task_time_dict
        task_time = DBManager.fetch_all_data(curs, sql)
        DBManager.destroy_db_connect(conn, curs)
        if task_time and task_time[0]:
            for task in task_time:
                task_time_dict[task[0]] = (task[1], task[2])
        return task_time_dict
