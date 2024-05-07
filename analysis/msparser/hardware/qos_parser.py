#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.

import logging
import os
import struct

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from framework.offset_calculator import OffsetCalculator
from msmodel.hardware.qos_model import QosModel
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag


class ParsingQosData(MsMultiProcess):
    """
    parsing QoS data class
    """
    QOS_INFO = "qos.info"

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.qos_data = []
        self.qos_info = []
        self.sample_config = sample_config
        self._file_list = file_list.get(DataTag.QOS, [])
        self.project_path = sample_config.get("result_dir", "")
        file_list = [file_name for file_name in self._file_list if self.QOS_INFO not in file_name]
        self.calculate = OffsetCalculator(file_list, StructFmt.QOS_FMT_SIZE, self.project_path)
        self._model = QosModel(self.project_path, DBNameConstant.DB_QOS,
                               [DBNameConstant.TABLE_QOS_ORIGIN, DBNameConstant.TABLE_QOS_INFO])
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    @staticmethod
    def _update_qos_data(start_time: int, item: list, headers: list) -> tuple:
        item[0] = start_time + item[0] * NumberConstant.USTONS
        item[2] = '{}'.format('read' if item[2] == 0 else 'write')
        return tuple(headers + item)

    def read_binary_data(self: any, file_name: str, device_id: str, replay_id: str) -> int:
        """
        parsing qos data and insert into qos.db
        """
        status = NumberConstant.ERROR
        qos_file = PathManager.get_data_file_path(self.project_path, file_name)
        if not os.path.exists(qos_file):
            return status
        _file_size = os.path.getsize(qos_file)
        with FileOpen(qos_file, "rb") as qos_f:
            qos_data = self.calculate.pre_process(qos_f.file_reader, _file_size)
            struct_nums = _file_size // StructFmt.QOS_FMT_SIZE
            struct_data = struct.unpack(StructFmt.BYTE_ORDER_CHAR + StructFmt.QOS_FMT * struct_nums,
                                        qos_data)
            start_time = InfoConfReader().get_start_timestamp()
            for i in range(struct_nums):
                self.qos_data.append(self._update_qos_data(start_time, list(struct_data[i * 4:(i + 1) * 4]),
                                                           [device_id, replay_id]))
        return NumberConstant.SUCCESS

    def parse(self: any) -> None:
        """
        parsing data file
        """
        for file_name in self._file_list:
            if self.QOS_INFO in file_name:
                self._handle_qos_info(file_name)
                continue
            if is_valid_original_data(file_name, self.project_path):
                self._handle_original_data(file_name)

    def save(self: any) -> None:
        """
        save data to db
        :return: None
        """
        if self.qos_data and self.qos_info:
            with self._model as model:
                model.flush(self.qos_data, DBNameConstant.TABLE_QOS_ORIGIN)
                model.flush(self.qos_info, DBNameConstant.TABLE_QOS_INFO)

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        try:
            if self._file_list:
                self.parse()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as qos_err:
            logging.error(str(qos_err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _handle_original_data(self: any, file_name: str) -> None:
        device_id = InfoConfReader().get_device_id()
        logging.info("start parsing QoS data file: %s", file_name)
        status = self.read_binary_data(file_name, device_id, '0')  # replay is 0
        FileManager.add_complete_file(self.project_path, file_name)
        if status:
            logging.error('Insert QoS bandwidth data error.')

        logging.info("Create QoS DB finished!")

    def _handle_qos_info(self: any, file_name: str) -> None:
        """
        read qos info
        """
        delimiter = "="
        qos_info_path = PathManager.get_data_file_path(self.project_path, file_name)
        with FileOpen(qos_info_path, "r") as file:
            qos_info = [x.replace("\n", "") for x in file.file_reader.readlines(Constant.MAX_READ_FILE_BYTES)]
        if not qos_info:
            logging.error("No qos info Found!")
        self.qos_info = [x.split(delimiter) for x in qos_info]
