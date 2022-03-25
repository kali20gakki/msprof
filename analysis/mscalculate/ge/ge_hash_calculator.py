#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to calculate ge hash and parse it.
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""

from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_constant import MsvpConstant
from model.ge.ge_info_model import GeModel
from model.interface.view_model import ViewModel
from mscalculate.interface.icalculator import ICalculator
from profiling_bean.prof_enum.data_tag import DataTag
from viewer.ge_info_report import get_ge_hash_dict


class GeHashCalculator(ICalculator, MsMultiProcess):
    """
    class used to calculate ge hash info
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._ge_model = GeModel(self._project_path, [DBNameConstant.TABLE_GE_TASK])
        self._ge_data = MsvpConstant.MSVP_EMPTY_DATA

    def get_ge_task_data(self: any) -> None:
        """
        get ge data
        :return: None
        """
        model_view = ViewModel(self._project_path, DBNameConstant.DB_GE_INFO, [DBNameConstant.TABLE_GE_TASK])
        if not model_view.check_table():
            return
        data = model_view.get_all_data(DBNameConstant.TABLE_GE_TASK)
        if not data:
            return
        self._ge_data = data

    def update_data(self: any) -> None:
        """
        update ge data with hash data
        :return: None
        """
        hash_dict = get_ge_hash_dict(self._project_path)
        result_data = []
        for _data in self._ge_data:
            _data = list(_data)
            _data[1] = hash_dict.get(_data[1], _data[1])
            result_data.append(_data)
        self._ge_data = result_data

    def calculate(self: any) -> None:
        """
        get and update ge data
        :return: None
        """
        self.get_ge_task_data()
        self.update_data()

    def save(self: any) -> None:
        """
        save ge data into database
        :return: None
        """
        if self._ge_data:
            self._ge_model.init()
            self._ge_model.delete_table(DBNameConstant.TABLE_GE_TASK)
            self._ge_model.insert_data_to_db(DBNameConstant.TABLE_GE_TASK, self._ge_data)
            self._ge_model.finalize()

    def ms_run(self: any) -> None:
        """
        entrance for calculating ge
        :return: None
        """
        if self._file_list.get(DataTag.GE_TASK):
            self.calculate()
            self.save()
