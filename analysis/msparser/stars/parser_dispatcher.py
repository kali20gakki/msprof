#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

import importlib
import logging
import os

from msconfig.config_manager import ConfigManager
from common_func.ms_constant.str_constant import StrConstant
from common_func.path_manager import PathManager


class ParserDispatcher:
    """
    class used to dispatch diff kind data type.
    And read the mapping relationship from the stars configuration file.
    The mapping relationship between processor, fmt, db and table is recorded in the stars configuration file.
    """

    IS_DB_NEEDED_CLEAR = 'is_db_needed_clear'

    def __init__(self: any, result_dir: str) -> None:
        self.parser_map = {}
        self.parser_list = []
        self.result_dir = result_dir
        self.cfg_parser = None
        self.modules = importlib.import_module("msparser.stars")

    def init(self: any) -> None:
        """
        load stars config info and build parser map
        :return:
        """
        self.cfg_parser = ConfigManager.get(ConfigManager.STARS)
        self.build_parser_map()

    def build_parser_map(self: any) -> None:
        """
        build parser fmt map
        :return: NA
        """
        for parser in self.cfg_parser.sections():
            fmts = self.cfg_parser.get(parser, StrConstant.CONFIG_FMT).split(",")
            db_name = self.cfg_parser.get(parser, StrConstant.CONFIG_DB)
            is_db_needed_clear = 0
            if self.cfg_parser.has_option(parser, self.IS_DB_NEEDED_CLEAR):
                is_db_needed_clear = self.cfg_parser.get(parser, self.IS_DB_NEEDED_CLEAR)
            if os.path.exists(PathManager.get_db_path(self.result_dir, db_name)) and is_db_needed_clear:
                os.remove(PathManager.get_db_path(self.result_dir, db_name))
            table_list = self.cfg_parser.get(parser, StrConstant.CONFIG_TABLE).split(",")
            if hasattr(self.modules, parser):
                parser = getattr(self.modules, parser)(self.result_dir, db_name, table_list)
                for fmt in fmts:
                    self.parser_map[fmt.strip()] = parser

    def dispatch(self: any, func_type: str, data: bytes) -> None:
        """
        dispatch parser by func_type
        :param func_type: func_type
        :param data: binary data
        :return:NA
        """
        if func_type in self.parser_map:
            self.parser_map.get(func_type).handle(func_type, data)
        else:
            logging.error("Not support data type %s", func_type)

    def flush_all_parser(self: any) -> None:
        """
        flush all parser data to db
        :return: NA
        """
        for parser in self.parser_map:
            self.parser_map.get(parser).flush()
