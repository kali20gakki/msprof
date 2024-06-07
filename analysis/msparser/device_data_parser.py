#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.

import importlib
import logging
import os
import sys

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.profiling_scene import ProfilingScene


class DeviceDataParser(MsMultiProcess):
    """
    parser and calculator for device data
    """
    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

    def ms_run(self: any) -> None:
        """
        调用msprof_analysis.so
        """
        all_export_flag = ProfilingScene().is_all_export() and InfoConfReader().is_all_export_version()
        if ProfilingScene().is_cpp_parse_enable() and all_export_flag:
            sys.path.append(os.path.realpath(os.path.join(os.path.dirname(__file__), "..", "lib64")))
            logging.info("Device Data will be parsed by msprof_analysis.so!")
            msprof_analysis_module = importlib.import_module("msprof_analysis")
            msprof_analysis_module.parser.dump_device_data(os.path.dirname(self._project_path))
        else:
            logging.warning("Device Data will not be parsed by msprof_analysis.so!")
        return
