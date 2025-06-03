#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

import logging

from common_func.constant import Constant
from common_func.ms_multi_process import MsMultiProcess
from common_func.ms_constant.str_constant import StrConstant
from msmodel.hardware.netdev_stats_model import NetDevStatsModel
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag


class NetDevStatsParser(DataParser, MsMultiProcess):
    """
    parsing netdev stats data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)
        self._file_list = file_list.get(DataTag.NETDEV_STATS, [])
        self._device_id = sample_config.get(StrConstant.PARAM_DEVICE_ID, "0")
        self._origin_data = []

    def parse(self: any) -> None:
        """
        parsing data file
        :return: None
        """
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))
        self._origin_data = self.parse_plaintext_data(
            self._file_list, lambda data_lines: [(self._device_id, *line.split()) for line in data_lines])

    def save(self: any) -> None:
        """
        save data to db
        :return: None
        """
        if self._origin_data:
            with NetDevStatsModel(self._project_path) as model:
                model.flush(self._origin_data)

    def ms_run(self: any) -> None:
        """
        Entrance of NetDevStats parser.
        :return: None
        """
        try:
            if self._file_list:
                self.parse()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
