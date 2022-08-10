#!/usr/bin/python3
# coding=utf-8
"""
function: path manager
Copyright Huawei Technologies Co., Ltd. 2020. All rights reserved.
"""
import logging
import os

from common_func.constant import Constant


class PathManager:
    """
    file path manager
    """
    DATA = "data"
    LOG = "log"
    SQLITE = "sqlite"
    COLLECTION_LOG = "collection.log"
    DISPATCH = "dispatch"
    TIMELINE = 'timeline'
    SUMMARY = 'summary'
    SAMPLE_JSON = "sample.json"
    PROFILER = ".profiler"
    HCCL = "hccl"
    QUERY_CLUSTER = "query"

    @classmethod
    def get_data_dir(cls: any, result_dir: str) -> str:
        """
        get data directory in result directory
        """
        return cls.get_path_under_result_dir(result_dir, cls.DATA)

    @classmethod
    def get_data_file_path(cls: any, result_dir: str, file_name: str) -> str:
        """
        get data file path with file name
        """
        return os.path.join(cls.get_data_dir(result_dir), file_name)

    @classmethod
    def get_log_dir(cls: any, result_dir: str) -> str:
        """
        get log directory in result directory
        """
        return cls.get_path_under_result_dir(result_dir, cls.LOG)

    @classmethod
    def get_sql_dir(cls: any, result_dir: str) -> str:
        """
        get sqlite directory in result directory
        """
        return cls.get_path_under_result_dir(result_dir, cls.SQLITE)

    @classmethod
    def get_db_path(cls: any, result_dir: str, db_name: str) -> str:
        """
        get db name path in result directory
        """
        return cls.get_path_under_result_dir(result_dir, cls.SQLITE, db_name)

    @classmethod
    def get_collection_log_path(cls: any, result_dir: str) -> str:
        """
        get collection log path
        """
        return cls.get_path_under_result_dir(result_dir, cls.LOG, cls.COLLECTION_LOG)

    @classmethod
    def get_dispatch_dir(cls: any, result_dir: str) -> str:
        """
        get dispatch dir in install path
        :return: the dispatch dir
        """
        return cls.get_path_under_result_dir(result_dir, cls.PROFILER, cls.DISPATCH)

    @classmethod
    def get_summary_dir(cls: any, result_dir: str) -> str:
        """
        get summary dir in result directory
        :return: the summary dir
        """
        return cls.get_path_under_result_dir(result_dir, cls.SUMMARY)

    @classmethod
    def get_timeline_dir(cls: any, result_dir: str) -> str:
        """
        get timeline dir in result directory
        :return: the timeline dir
        """
        return cls.get_path_under_result_dir(result_dir, cls.TIMELINE)

    @classmethod
    def get_sample_json_path(cls: any, result_dir: str) -> str:
        """
        get sample json path in result directory
        """
        return cls.get_path_under_result_dir(result_dir, cls.SAMPLE_JSON)

    @classmethod
    def get_hccl_path(cls: any, result_dir: str) -> str:
        """
        get hccl trace path in result directory
        """
        return cls.get_path_under_result_dir(result_dir, cls.HCCL)

    @staticmethod
    def get_path_under_result_dir(result_dir: str, *paths: str) -> str:
        """
        get directory path
        """
        if result_dir is not None:
            return os.path.join(result_dir, *paths)
        return ""

    @staticmethod
    def check_map_file_path(map_file_path: str, cfg_parser: any) -> bool:
        map_file_path = os.path.realpath(map_file_path)
        try:
            if os.path.getsize(map_file_path) <= Constant.MAX_READ_FILE_BYTES:
                cfg_parser.read(map_file_path)
                return True
            return False
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as error:
            logging.error(str(error), exc_info=Constant.TRACE_BACK_SWITCH)
            return False
