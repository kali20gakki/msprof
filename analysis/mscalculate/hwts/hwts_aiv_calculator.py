#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to calculate hwts aiv offset and parse it.
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""

from analyzer.scene_base.profiling_scene import ProfilingScene
from analyzer.op_common_function import OpCommonFunc
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.utils import Utils
from common_func.constant import Constant
from common_func.batch_counter import BatchCounter
from model.task_time.hwts_aiv_model import HwtsAivModel
from mscalculate.hwts.hwts_calculator import HwtsCalculator
from profiling_bean.prof_enum.data_tag import DataTag


class HwtsAivCalculator(HwtsCalculator):
    """
    class used to calculate hwts offset and parse log by iter
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(file_list, sample_config)
        self._file_list = file_list.get(DataTag.HWTS_AIV, [])
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))
        self._hwts_aiv_model = HwtsAivModel(self._project_path,
                                            [DBNameConstant.TABLE_HWTS_TASK,
                                             DBNameConstant.TABLE_HWTS_TASK_TIME])

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return "HwtsAivCalculator"

    def ms_run(self: any) -> None:
        """
        entrance for aiv calculator
        :return: None
        """
        if self._file_list and ProfilingScene().is_operator():
            self._parse_all_file()
            self.save()

    def save(self: any) -> None:
        """
        save hwts data
        :return: None
        """
        self._hwts_aiv_model.clear()
        if self._log_data:
            self._hwts_aiv_model.init()
            self._hwts_aiv_model.flush_data(Utils.obj_list_to_list(self._log_data), DBNameConstant.TABLE_HWTS_TASK)
            self._hwts_aiv_model.flush_data(self._add_batch_id(self._prep_data()), DBNameConstant.TABLE_HWTS_TASK_TIME)
            self._hwts_aiv_model.finalize()

    def _add_batch_id(self: any, prep_data_res: list) -> list:
        batch_counter = BatchCounter(self._project_path)
        batch_counter.init(Constant.TASK_TYPE_AIV)
        for index, datum in enumerate(prep_data_res):
            # index 0 stream id, index 1 task id
            batch_id = batch_counter.calculate_batch(datum[0], datum[1])
            prep_data_res[index] = list(datum[:2]) + [
                InfoConfReader().time_from_syscnt(datum[2]),
                InfoConfReader().time_from_syscnt(datum[3]),
                self._sample_config.get('iter_id'),
                self._sample_config.get('model_id'),
                batch_id]
        return prep_data_res
