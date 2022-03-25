#!/usr/bin/python3
# coding:utf-8
"""
This scripts is used to get export data configs
Copyright Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
"""
import configparser
import os
import threading

from common_func.msvp_common import MsvpCommonConst
from common_func.constant import Constant
from common_func.ms_constant.str_constant import StrConstant


class GetExportDataConfigs:
    """
    get export data configs from ExportData.ini
    """
    cfg_parser = None
    init_cfg_finished = False
    lock = threading.Lock()

    @classmethod
    def init_data(cls: any, load_conf: str = StrConstant.EXPORTDATA_INI) -> None:
        """
        export data, support different data types and export types
        :param load_conf: conf file
        :return: None
        """
        if not cls.init_cfg_finished:
            cls.lock.acquire()
            if not cls.init_cfg_finished:
                cls.load_export_data_config(load_conf)
            cls.lock.release()

    @classmethod
    def load_export_data_config(cls: any, load_conf: str) -> None:
        """
        load export configuration
        :param load_conf: conf file
        :return: None
        """
        config_file_path = os.path.join(MsvpCommonConst.CONFIG_PATH, load_conf)
        if os.path.exists(config_file_path) and os.path.getsize(
                config_file_path) <= Constant.MAX_READ_FILE_BYTES:
            cls.cfg_parser = configparser.ConfigParser(interpolation=None)
            cls.cfg_parser.read(config_file_path)
            cls.init_cfg_finished = True

    @classmethod
    def get_used_columns(cls: any, data_type: str, load_conf: str = StrConstant.EXPORTDATA_INI) -> str:
        """
        get used columns
        :param data_type: data type
        :param load_conf: conf file
        :return: cols
        """
        cls.init_data(load_conf)
        cols = ""
        if cls.cfg_parser:
            if cls.cfg_parser.has_option(data_type, StrConstant.CONFIG_COLUMNS):
                cols = cls.cfg_parser.get(data_type, StrConstant.CONFIG_COLUMNS)
        return cols

    @classmethod
    def get_data_headers(cls: any, data_type: str, load_conf: str = StrConstant.EXPORTDATA_INI) -> list:
        """
        get data headers
        :param data_type: data type
        :param load_conf: conf file
        :return: header list
        """
        cls.init_data(load_conf)
        if cls.cfg_parser:
            if cls.cfg_parser.has_option(data_type, StrConstant.CONFIG_HEADERS):
                headers = cls.cfg_parser.get(data_type, StrConstant.CONFIG_HEADERS)
                if headers:
                    return headers.split(",")
        return []

    @classmethod
    def get_table_name(cls: any, data_type: str, load_conf: str = StrConstant.EXPORTDATA_INI) -> str:
        """
        get data tables
        :param data_type: data type
        :param load_conf: conf file
        :return: table name
        """
        cls.init_data(load_conf)
        table_name = ""
        if cls.cfg_parser:
            if cls.cfg_parser.has_option(data_type, StrConstant.CONFIG_TABLE):
                table_name = cls.cfg_parser.get(data_type, StrConstant.CONFIG_TABLE)
        return table_name

    @classmethod
    def get_db_name(cls: any, data_type: str, load_conf: str = StrConstant.EXPORTDATA_INI) -> str:
        """
        get db name
        :param data_type: data type
        :param load_conf: conf file
        :return: db name
        """
        cls.init_data(load_conf)
        db_name = ""
        if cls.cfg_parser:
            if cls.cfg_parser.has_option(data_type, StrConstant.CONFIG_DB):
                db_name = cls.cfg_parser.get(data_type, StrConstant.CONFIG_DB)
        return db_name

    @classmethod
    def get_all_data_types(cls: any, load_conf: str = StrConstant.EXPORTDATA_INI) -> list:
        """
        get all data types
        :param load_conf: conf file
        :return: data types
        """
        cls.init_data(load_conf)
        if cls.cfg_parser:
            return cls.cfg_parser.sections()
        return []
