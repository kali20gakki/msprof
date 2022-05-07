#!/usr/bin/python3
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import os

from common_func.msvp_constant import MsvpConstant
from common_func.ms_constant.str_constant import StrConstant
from framework.config_data_parsers import ConfigDataParsers
from framework.iprof_factory import IProfFactory


class ParserFactory(IProfFactory):
    """
    factory for create data parser
    """
    CONF_FILE = os.path.join(MsvpConstant.CONFIG_PATH, 'data_parsers.ini')

    def __init__(self: any, file_list: dict, sample_config: dict, chip_model: str) -> None:
        self._sample_config = sample_config
        self._file_list = file_list
        self._chip_model = chip_model
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

    def generate(self: any, chip_model: any) -> dict:
        """
        generate parser
        """
        return ConfigDataParsers.get_parsers(chip_model, self.CONF_FILE)

    def run(self: any) -> None:
        """
        run function
        """
        self._launch_parser_list(self._sample_config, self._file_list, self.generate(self._chip_model))
