"""
This script is used to create ai core operator op_summary db from other db.
Copyright Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
"""

from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from msmodel.stars.op_summary_model import OpSummaryModel
from mscalculate.interface.icalculator import ICalculator


class OpSummaryCalculator(ICalculator, MsMultiProcess):
    """
    create op op_summary db.
    """

    def __init__(self: any, _: dict, sample_config):
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._result_dir = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

    def ms_run(self: any) -> None:
        """
        entrance for calculating op_summary
        :return: None
        """
        self.calculate()

    def calculate(self: any) -> None:
        """
        get and update data
        :return: None
        """
        with OpSummaryModel(self._sample_config) as model:
            model.create_table()

    def save(self: any) -> None:
        """
        save data into database
        :return: None
        """
        self.calculate()
