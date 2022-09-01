#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to calculate data queue data.
Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
"""

import logging
import sqlite3

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.os_manager import check_file_readable
from common_func.path_manager import PathManager
from msmodel.ai_cpu.data_queue_model import DataQueueModel
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag


class DataQueueParser(IParser, MsMultiProcess):
    """
    class used to calculate data queue in cluster
    """

    AIC_PMU_SIZE = 128

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list.get(DataTag.DATA_QUEUE, [])
        self._sample_config = sample_config
        self._project_path = self._sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._model = DataQueueModel(self._project_path, [DBNameConstant.TABLE_DATA_QUEUE])
        self._data_list = []

    @staticmethod
    def _read_data(file_path: str) -> list:
        check_file_readable(file_path)
        lines = []
        with open(file_path, 'rb') as file_reader:
            line = str(file_reader.read().replace(b'\n\x00', b' ___ ').replace(b'\x00', b' ___ '),
                       encoding='utf-8')
            lines += list(filter(None, line.split(" ___ ")))
        return lines

    def parse(self: any) -> None:
        """
        to read data queue
        :return: None
        """
        try:
            for _file in self._file_list:
                file_path = PathManager.get_data_file_path(self._project_path, _file)
                self._parse_file(file_path)
        except (OSError, SystemError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def save(self: any) -> None:
        """
        save and parser data to db
        :return: None
        """
        if not self._data_list:
            logging.warning("No data preprocess data, data list is empty!")
            return

        try:
            with self._model as _model:
                _model.flush(self._data_list)
        except sqlite3.Error as err:
            logging.error("Save data queue failed! %s", err)

    def ms_run(self: any) -> None:
        """
        parse data queue and save it to db.
        :return:None
        """
        if self._file_list:
            self.parse()
            self.save()

    def _parse_file(self: any, file_path: str) -> None:
        """
        read json data
        :param file_path:
        :return:
        """
        data_list = self._read_data(file_path)
        try:
            for data in data_list:
                data_line = data.split(',')
                res_data = [key_value.split(':')[1] for key_value in data_line]
                res_data[1:] = list(map(int, res_data[1:]))
                res_data.append(res_data[3] - res_data[2])
                self._data_list.append(res_data)
        except (ValueError, IndexError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
