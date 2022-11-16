#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import hashlib
import logging
import multiprocessing
import os
import time

from common_func.common import CommonConstant
from common_func.common import LogFactory
from common_func.common import call_sys_exit
from common_func.common import generate_config
from common_func.common import print_info
from common_func.constant import Constant
from common_func.data_check_manager import DataCheckManager
from common_func.file_manager import FileManager
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_common import MsProfCommonConstant, update_sample_json
from common_func.msprof_common import add_all_file_complete
from common_func.msprof_common import check_path_valid
from common_func.msprof_exception import ProfException
from common_func.msvp_common import clear_project_dirs
from common_func.path_manager import PathManager
from common_func.utils import Utils
from framework.collection_engine import AI
from framework.file_dispatch import FileDispatch
from framework.load_info_manager import LoadInfoManager


class JobDispatcher:
    """
    The class for job dispatcher
    """
    # root directory
    SCRIPT_NAME = os.path.basename(__file__)

    def __init__(self: any, collection_path: str) -> None:
        self.collection_path = os.path.realpath(collection_path)
        self.dispatch_path = PathManager.get_dispatch_dir(self.collection_path)

    def add_symlink(self: any) -> None:
        """
        add symlink for jobxxx
        :return: None
        """
        check_path_valid(self.dispatch_path, True)
        self._clean_invalid_dispatch_symlink()
        for job_tag in os.listdir(self.collection_path):
            self._add_symlink_by_job_tag(job_tag)

    def process(self: any) -> None:
        """
        process dispatch
        :return: None
        """
        print_info(self.SCRIPT_NAME, "Start running profiling dispatch client.")
        try:
            while True:
                try:
                    self.add_symlink()
                except (OSError, SystemError, ValueError, TypeError,
                        RuntimeError, ProfException) as err:
                    logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
                # dispatch process cycle interval:3s
                time.sleep(3)
        except KeyboardInterrupt:
            print_info(self.SCRIPT_NAME, "KeyboardInterrupt")
            call_sys_exit(0)
        finally:
            pass

    def _add_symlink_by_job_tag(self: any, job_tag: str) -> None:
        job_realpath = os.path.realpath(os.path.join(self.collection_path, job_tag))
        try:
            if DataCheckManager.contain_info_json_data(job_realpath):
                residue = hashlib.md5()
                residue.update(job_tag.encode("utf-8"))
                client_num = int(residue.hexdigest(), 16) % CommonConstant.CLIENT_NUM
                client_num_path = \
                    os.path.join(self.dispatch_path, str(client_num))
                job_path = os.path.join(client_num_path, job_tag)
                if os.path.isdir(job_path):
                    return
                if os.path.isfile(job_path):
                    print_info(self.SCRIPT_NAME, 'The path "%s" is invalid, please '
                                                 'check the path.' % job_path)
                    return
                check_path_valid(client_num_path, True)
                os.symlink(os.path.join(self.collection_path, job_tag),
                           job_path)
        except (OSError, SystemError, ValueError, TypeError,
                RuntimeError, ProfException) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)

    def _clean_invalid_dispatch_symlink_by_client_num(self: any, client_num: str) -> None:
        client_num_path = os.path.join(self.dispatch_path, client_num)
        try:
            for job_tag in os.listdir(client_num_path):
                symlink_file = os.path.join(client_num_path, job_tag)
                if not os.path.isdir(symlink_file) and os.path.islink(
                        symlink_file):
                    os.remove(symlink_file)
        except (OSError, SystemError, ValueError, TypeError,
                RuntimeError, ProfException) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)

    def _clean_invalid_dispatch_symlink(self: any) -> None:
        """
        dispatch_path: the dispatch path
        clean up the invalid symlink files.
        """
        if not os.path.exists(self.dispatch_path):
            return
        for client_num in os.listdir(self.dispatch_path):
            self._clean_invalid_dispatch_symlink_by_client_num(client_num)


class JobMonitor:
    """
    The class for monitor dispatch job
    """
    SCRIPT_NAME = os.path.basename(__file__)

    def __init__(self: any, monitor_path: str) -> None:
        self.monitor_path = monitor_path

    @staticmethod
    def _launch_parsing_job_data(result_dir: str, sample_config: dict, tag_id: str) -> None:
        job_sample_config = {
            "result_dir": result_dir, "tag_id": tag_id, "host_id": MsProfCommonConstant.DEFAULT_IP,
            "sql_path": PathManager.get_sql_dir(result_dir)
        }
        target_collection = AI(
            {**sample_config, **job_sample_config})
        target_collection.import_control_flow()
        update_sample_json(sample_config, result_dir)
        file_dispatch = FileDispatch({**sample_config, **job_sample_config})
        file_dispatch.dispatch_parser()
        add_all_file_complete(result_dir)

    def analyze_data(self: any) -> None:
        """
        analyze data
        :return: None
        """
        try:
            for job_tag in os.listdir(self.monitor_path):
                job_path = os.path.realpath(
                    os.path.join(self.monitor_path, job_tag))
                job_tag = os.path.basename(job_path)
                if DataCheckManager.contain_info_json_data(job_path):
                    LoadInfoManager().load_info(job_path)
                    self._analysis_job_profiling(job_path, job_tag)
        except (OSError, SystemError, ValueError, TypeError,
                RuntimeError, ProfException) as error:
            logging.error(str(error), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass

    def process(self: any) -> None:
        """
        process monitor
        """
        print_info(self.SCRIPT_NAME, "Start running profiling monitor client.")
        try:
            while True:
                if os.path.exists(self.monitor_path):
                    self.analyze_data()
                time.sleep(5)  # sleep 5 seconds
        except KeyboardInterrupt:
            print_info(self.SCRIPT_NAME, "KeyboardInterrupt\n")
            call_sys_exit(NumberConstant.SUCCESS)

    def _analysis_job_profiling(self: any, job_path: str, job_tag: str) -> None:
        sample_file = PathManager.get_sample_json_path(job_path)
        _ = LogFactory().get_logger(job_tag, job_path)
        if not os.path.exists(sample_file):
            logging.error('"%s" does not exist under "%s".', CommonConstant.SAMPLE_JSON,
                          job_tag)
            return
        if not FileManager.is_analyzed_data(job_path):
            sample_config = generate_config(sample_file)
            check_path_valid(PathManager.get_sql_dir(job_path), True)
            clear_project_dirs(job_path)
            self._launch_parsing_job_data(job_path, sample_config, job_tag)


def _monitor_job(num: int, collection_path: str) -> None:
    dispatch_path = PathManager.get_dispatch_dir(os.path.relpath(collection_path))
    monitor_path = os.path.join(dispatch_path, str(num))
    job_monitor = JobMonitor(monitor_path)
    job_monitor.process()


def _dispatch_job(collection_path: str) -> None:
    job_dispatch = JobDispatcher(collection_path)
    job_dispatch.process()


def monitor(collection_path: str) -> None:
    """
    monitor the collection path
    :param collection_path: collection path
    :return: None
    """
    check_path_valid(collection_path, False)
    try:
        _monitor_process(collection_path)
    except KeyboardInterrupt:
        print_info(JobDispatcher.SCRIPT_NAME, "Parent stopped ...")


def _monitor_process(collection_path: str) -> None:
    processes = Utils.generator_to_list(multiprocessing.Process(target=_monitor_job, args=(i, collection_path,))
                                        for i in range(CommonConstant.CLIENT_NUM))
    processes.append(multiprocessing.Process(target=_dispatch_job,
                                             args=(collection_path,)))
    for process in processes:
        process.start()
    for process in processes:
        process.join()
