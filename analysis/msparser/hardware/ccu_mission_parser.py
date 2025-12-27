# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

import logging
import os

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from framework.offset_calculator import OffsetCalculator
from msmodel.hardware.ccu_mission_model import CCUMissionModel
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.hardware.ccu_bean import CCUMissionBean


class CCUMissionParser(MsMultiProcess):
    """
    class used to parser ccu mission data
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._file_list = file_list.get(DataTag.CCU_MISSION, [])
        self._project_path = sample_config.get("result_dir", "")
        self._calculate = OffsetCalculator(self._file_list, StructFmt.CCU_MISSION_FMT_SIZE, self._project_path)
        self._model = CCUMissionModel(self._project_path, DBNameConstant.DB_CCU, [DBNameConstant.TABLE_CCU_MISSION])
        self.mission_data = []

    @staticmethod
    def get_single_mission_data(dev_id, end_time, single_bean):
        return [single_bean.stream_id,
                single_bean.task_id,
                single_bean.lp_instr_id,
                single_bean.lp_start_time,
                single_bean.lp_end_time,
                single_bean.setckebit_instr_id,
                single_bean.setckebit_start_time,
                dev_id,
                end_time]

    def read_binary_data(self: any, file_name: str) -> int:
        """
        parsing ccu mission data
        """
        files = PathManager.get_data_file_path(self._project_path, file_name)
        if not os.path.exists(files):
            return NumberConstant.ERROR
        _file_size = os.path.getsize(files)
        if _file_size < StructFmt.CCU_MISSION_FMT_SIZE:
            logging.error("CCU mission data struct is incomplete, please check the file.")
            return NumberConstant.ERROR
        with FileOpen(files, "rb") as f:
            mission_bin = self._calculate.pre_process(f.file_reader, _file_size)
            struct_nums = _file_size // StructFmt.CCU_MISSION_FMT_SIZE
            for i in range(struct_nums):
                single_bean = CCUMissionBean().decode(
                    mission_bin[i * StructFmt.CCU_MISSION_FMT_SIZE: (i + 1) * StructFmt.CCU_MISSION_FMT_SIZE])
                self.get_mission_data(single_bean)
        return NumberConstant.SUCCESS

    def get_mission_data(self, single_bean):
        if single_bean and len(single_bean.rel_end_time) > 0:
            for dev_id, end_time in single_bean.rel_end_time:
                self.mission_data.append(self.get_single_mission_data(dev_id, end_time, single_bean))
        elif single_bean and len(single_bean.rel_end_time) == 0:
            self.mission_data.append(
                self.get_single_mission_data(Constant.DEFAULT_VALUE, Constant.DEFAULT_VALUE, single_bean))

    def parse(self: any) -> None:
        """
        parsing data file
        """
        logging.info("Start parsing CCU mission data file.")
        for file_name in self._file_list:
            if is_valid_original_data(file_name, self._project_path):
                self._handle_original_data(file_name)
        logging.info("Parse CCU mission data finished!")

    def save(self: any) -> None:
        """
        save data to db
        :return: None
        """
        if self.mission_data:
            with self._model as model:
                model.flush(self.mission_data, DBNameConstant.TABLE_CCU_MISSION)

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        try:
            if self._file_list:
                self.parse()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _handle_original_data(self: any, file_name: str) -> None:
        if self.read_binary_data(file_name) == NumberConstant.ERROR:
            logging.error('Parse CCU mission data file: %s error.', file_name)
        FileManager.add_complete_file(self._project_path, file_name)
