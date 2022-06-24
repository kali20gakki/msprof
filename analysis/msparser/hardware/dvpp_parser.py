# -*-coding:utf-8 -*-
"""
parsing peripheral data file and create peripheral db
"""
import logging
import os

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.utils import Utils
from msmodel.hardware.dvpp_model import DvppModel
from profiling_bean.prof_enum.data_tag import DataTag


class ParsingPeripheralData(MsMultiProcess):
    """
    parsing peripheral data class
    """

    DATA_LENGTH = 11

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self.replayid = 0
        self.device_id = self.sample_config.get("device_id", "0")
        self.has_dvpp_id = False
        self._file_list = file_list.get(DataTag.DVPP, [])
        self.dvpp_data = []
        self.project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._model = \
            DvppModel(self.project_path, DBNameConstant.DB_PERIPHERAL,
                      [DBNameConstant.TABLE_DVPP_ORIGIN, DBNameConstant.TABLE_DVPP_REPORT])
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    def dvpp_data_parsing(self: any, binary_data_path: str) -> None:
        """
        parsing dvpp data file
        """
        if "dvpp" not in binary_data_path:
            return
        try:
            with open(os.path.join(self.project_path,
                                   "data", binary_data_path), 'r') as dvpp_file:
                lines = dvpp_file.readlines(Constant.MAX_READ_FILE_BYTES)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s: %s", binary_data_path, str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self._read_binary_helper(lines)

    def start_parsing_data_file(self: any) -> None:
        """
        start parsing data file
        """
        try:
            for file_name in self._file_list:
                if is_valid_original_data(file_name, self.project_path):
                    logging.info(
                        "start parsing peripheral data file: %s", file_name)
                    self.dvpp_data_parsing(file_name)
                    FileManager.add_complete_file(self.project_path, file_name)
        except (SystemError, ValueError, TypeError, RuntimeError, OSError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        logging.info("Create Peripheral DB finished!")

    def save(self: any) -> None:
        """
        save data to db
        :return: None
        """
        if self.dvpp_data and self._model:
            self._model.init()
            self._model.create_table()
            self._model.flush(self.dvpp_data)
            self._model.report_data(self.has_dvpp_id)
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        try:
            if self._file_list:
                self.start_parsing_data_file()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as dvpp_err:
            logging.error(str(dvpp_err))

    def _generate_dvpp_data(self: any, dvpp_data: list) -> None:
        _dvpp_data = []
        if not self.has_dvpp_id:
            for x in dvpp_data:
                _dvpp_data.append([self.device_id, self.replayid, float(
                    x[0].replace(":", '.')), 0] + x[1:])
        else:
            for x in dvpp_data:
                _dvpp_data.append([self.device_id, self.replayid, float(
                    x[0].replace(":", '.'))] + x[1:])
        self.dvpp_data = _dvpp_data

    def _read_binary_helper(self: any, lines: list) -> None:
        dvpp_data = Utils.generator_to_list(x.split() for x in lines)

        if len(dvpp_data) > 0:
            for item in dvpp_data[0]:
                if item == "dvpp_id":
                    self.has_dvpp_id = True
                    break
        else:
            logging.warning("No Dvpp Data Found!")
            return
        dvpp_list = [x for x in dvpp_data if (x[1].isdigit() and len(x) == int(self.has_dvpp_id) + self.DATA_LENGTH)]
        dvpp_data = \
            Utils.generator_to_list(dvpp_list)
        if not dvpp_data:
            logging.warning("No Dvpp Data Found!")
            return
        self._generate_dvpp_data(dvpp_data)
