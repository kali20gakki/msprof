# coding=utf-8
"""
function: script used to parse ffts pmu data and save it to db
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""
import logging
import os
import sqlite3

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.os_manager import check_file_readable
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.utils import Utils
from framework.offset_calculator import OffsetCalculator
from model.stars.ffts_pmu_model import FftsPmuModel
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.stars.ffts_plus_pmu import FftsPlusPmuBean
from profiling_bean.stars.ffts_pmu import FftsPmuBean


class FftsPmuParser(IParser, MsMultiProcess):
    """
    class used to parse ffts pmu data
    """
    AIC_PMU_SIZE = 128

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self._file_list = file_list.get(DataTag.FFTS_PMU, [])
        self._sample_config = sample_config
        self._project_path = self._sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._model = FftsPmuModel(self._project_path, DBNameConstant.DB_RUNTIME, [])
        self._decoder = self._get_pmu_decoder()
        self._data_list = []
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    @classmethod
    def _get_pmu_decoder(cls: any) -> any:
        if ChipManager().is_ffts_plus_type():
            return FftsPlusPmuBean
        return FftsPmuBean

    def parse(self: any) -> None:
        """
        to read ffts pmu data
        :return: None
        """
        try:
            for _file in self._file_list:
                file_path = PathManager.get_data_file_path(self._project_path, _file)
                self._parse_binary_file(file_path)
        except (OSError, SystemError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def save(self: any) -> None:
        """
        save parser data to db
        :return: None
        """
        if not self._data_list:
            logging.warning("No legal ai core pmu data, data list is empty!")
            return

        try:
            with self._model as _model:
                _model.flush(self._data_list)
        except sqlite3.Error as err:
            logging.error("Save ffts pmu data failed! %s", err)
        finally:
            pass

    def ms_run(self: any) -> None:
        """
        parse ffts pmu data and save it to db.
        :return:None
        """

        if self._sample_config.get('ai_core_profiling_mode') == 'sample-based':
            return

        if self._file_list:
            self.parse()
            self.save()

    def _parse_binary_file(self: any, file_path: str) -> None:
        """
        read binary data an decode
        :param file_path:
        :return:
        """
        check_file_readable(file_path)
        offset_calculator = OffsetCalculator(self._file_list, self.AIC_PMU_SIZE, self._project_path)
        with FileOpen(file_path, 'rb') as _pmu_file:
            _file_size = os.path.getsize(file_path)
            file_data = offset_calculator.pre_process(_pmu_file.file_reader, _file_size)
            for chunk in Utils.chunks(file_data, self.AIC_PMU_SIZE):
                self._data_list.append(self._decoder.decode(chunk))
