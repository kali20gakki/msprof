#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import logging
import os
import sqlite3

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.common import generate_config
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_iteration import MsprofIteration
from common_func.os_manager import check_file_readable
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.utils import Utils
from framework.offset_calculator import FileCalculator
from framework.offset_calculator import OffsetCalculator
from mscalculate.interface.icalculator import ICalculator
from msmodel.iter_rec.iter_rec_model import HwtsIterModel
from msmodel.stars.ffts_pmu_model import FftsPmuModel
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.stars.ffts_plus_pmu import FftsPlusPmuBean
from profiling_bean.stars.ffts_pmu import FftsPmuBean


class FftsPmuCalculate(ICalculator, MsMultiProcess):
    """
    class used to parse ffts pmu data
    """
    FFTS_PMU_SIZE = 128

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._result_dir = self._sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._iter_model = HwtsIterModel(self._result_dir)
        self._model = FftsPmuModel(self._result_dir, DBNameConstant.DB_RUNTIME, [])
        self._decoder = self._get_pmu_decoder()
        self._data_list = []
        self._file_list = file_list.get(DataTag.FFTS_PMU, [])
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    @classmethod
    def _get_pmu_decoder(cls: any) -> any:
        if ChipManager().is_ffts_type():
            return FftsPmuBean
        return FftsPlusPmuBean

    def ms_run(self: any) -> None:
        config = generate_config(PathManager.get_sample_json_path(self._result_dir))
        if config.get(StrConstant.AICORE_PROFILING_MODE) == 'sample-based' \
                or config.get(StrConstant.AIV_PROFILING_MODE) == 'sample-based':
            return
        self.calculate()
        self.save()

    def calculate(self: any) -> None:
        if ProfilingScene().is_operator():
            self._parse_all_file()
        else:
            self._parse_by_iter()

    def save(self: any) -> None:
        """
        save parser data to db
        :return: None
        """
        if not self._data_list:
            logging.warning("No ai core pmu data found, data list is empty!")
            return

        try:
            with self._model as _model:
                _model.flush(self._data_list)
        except sqlite3.Error as err:
            logging.error("Save ffts pmu data failed! %s", err)

    def _need_to_analyse(self: any) -> bool:
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_RUNTIME)
        if not os.path.exists(db_path):
            return True
        if DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_METRICS_SUMMARY):
            return False
        return True

    def _parse_all_file(self):
        if not self._need_to_analyse():
            return
        try:
            for _file in self._file_list:
                file_path = PathManager.get_data_file_path(self._result_dir, _file)
                self._parse_binary_file(file_path)
        except (OSError, SystemError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _parse_by_iter(self):
        runtime_db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_RUNTIME)
        if os.path.exists(runtime_db_path):
            with self._model as model:
                model.drop_table(DBNameConstant.TABLE_METRICS_SUMMARY)
        with self._iter_model as iter_model:
            if not iter_model.check_db() or not iter_model.check_table():
                return
            # iter_id means [iter_id-1, iter_id]
            _iter_id = MsprofIteration(self._result_dir). \
                get_iter_id_by_index_id(self._sample_config.get(StrConstant.PARAM_ITER_ID),
                                        self._sample_config.get(StrConstant.PARAM_MODEL_ID))
            offset_count, total_count = self._get_offset_and_total(_iter_id[0] + 1)
            if total_count <= 0:
                logging.warning("The ffts pmu data that is not satisfied by the specified iteration!")
                return
            _file_calculator = FileCalculator(self._file_list, self.FFTS_PMU_SIZE, self._result_dir,
                                              offset_count, total_count)
            for chunk in Utils.chunks(_file_calculator.prepare_process(), self.FFTS_PMU_SIZE):
                self._data_list.append(self._decoder.decode(chunk))

    def _get_total_aic_count(self: any) -> int:
        sum_file_size = 0
        for file in self._file_list:
            sum_file_size += os.path.getsize(PathManager.get_data_file_path(self._result_dir, file))
        return sum_file_size // self.FFTS_PMU_SIZE

    def _get_offset_and_total(self: any, iter_id: int) -> (int, int):
        """
        :param iter_id:
        :return: offset count and total aic count
        """
        offset_count, total_count = self._iter_model.get_task_offset_and_sum(iter_id, 'ai_core_num')
        _total_aic_count = self._get_total_aic_count()
        _sql_aic_count = self._iter_model.get_aic_sum_count()
        # get offset by all aic count and sql record count
        offset_count = _total_aic_count - _sql_aic_count + offset_count
        if offset_count < 0:
            total_count += offset_count
            offset_count = 0
        return offset_count, total_count

    def _parse_binary_file(self: any, file_path: str) -> None:
        """
        read binary data an decode
        :param file_path:
        :return:
        """
        check_file_readable(file_path)
        offset_calculator = OffsetCalculator(self._file_list, self.FFTS_PMU_SIZE, self._result_dir)
        with FileOpen(file_path, 'rb') as _pmu_file:
            _file_size = os.path.getsize(file_path)
            file_data = offset_calculator.pre_process(_pmu_file.file_reader, _file_size)
            for chunk in Utils.chunks(file_data, self.FFTS_PMU_SIZE):
                self._data_list.append(self._decoder.decode(chunk))
