#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import configparser
import importlib

from msconfig.config_manager import ConfigManager
from common_func.os_manager import check_file_readable
from common_func.utils import Utils


class ConfigDataParsers:
    """
    get data parsers from config file
    """

    KEY_PATH = "path"
    KEY_CHIP_MODEL = "chip_model"
    KEY_LEVEL = "level"
    DEFAULT_PARSER_LEVEL = "1"

    @classmethod
    def get_parsers(cls: any, config_name: str, chip_model: str) -> dict:
        """
        get parsers by chip model
        :param config_name: DataParsersConfig, DataCalculatorConfig
        :param chip_model: 0,1,2
        :return: data parsers
        """
        parsers_dict = {}
        conf_parser_read = ConfigManager.get(config_name)
        parser_section = conf_parser_read.sections()
        for _section in parser_section:
            chip_model_list = cls._load_parser_chip_model(conf_parser_read, _section)
            if chip_model not in chip_model_list:
                continue
            parser_level = cls._load_parser_level(conf_parser_read, _section)
            parsers_dict.setdefault(parser_level, []).append(cls._load_parser_module(conf_parser_read, _section))
        parsers_dict = dict(sorted(parsers_dict.items(), key=lambda x: int(x[0])))
        return parsers_dict

    @classmethod
    def load_conf_file(cls: any, config_file_path: str) -> any:
        """
        load conf file
        :param config_file_path: config file path
        :return: ConfigParser
        """
        check_file_readable(config_file_path)
        conf_parser_read = configparser.ConfigParser()
        conf_parser_read.read(config_file_path)
        return conf_parser_read

    @classmethod
    def _load_parser_level(cls: any, conf_parser_read: any, section: str) -> str:
        """
        get parser level
        :param conf_parser_read:config parser object
        :param section:parser name config in the config file
        :return: parser level
        """
        parser_level = cls.DEFAULT_PARSER_LEVEL
        if conf_parser_read.has_option(section, cls.KEY_LEVEL):
            parser_level = conf_parser_read.get(section, cls.KEY_LEVEL)
        return parser_level

    @classmethod
    def _load_parser_chip_model(cls: any, conf_parser_read: any, section: str) -> list:
        """
        load chip model from per parser
        :param conf_parser_read:config parser object
        :param section:parser name config in the config file
        :return: chip model list
        """
        chip_model_list = []
        if conf_parser_read.has_option(section, cls.KEY_CHIP_MODEL):
            chip_models = conf_parser_read.get(section, cls.KEY_CHIP_MODEL)
            chip_model_list = Utils.generator_to_list(_chip.strip() for _chip in chip_models.strip().split(","))
        return chip_model_list

    @classmethod
    def _load_parser_module(cls: any, conf_parser_read: any, section: str) -> any:
        """
        load data parser
        :param conf_parser_read: config parser object
        :param section: parser name config in the config file
        :return: parsers
        """
        if conf_parser_read.has_option(section, cls.KEY_PATH):
            parser_path = conf_parser_read.get(section, cls.KEY_PATH)
            parser_module = importlib.import_module(parser_path)
            if hasattr(parser_module, section):
                parser_obj = getattr(parser_module, section)
                return parser_obj
        return []
