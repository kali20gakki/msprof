#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
import logging
import multiprocessing
import time
from abc import abstractmethod

from common_func.common import init_log
from common_func.constant import Constant
from framework.load_info_manager import LoadInfoManager


class MsMultiProcess(multiprocessing.Process):
    """
    ms multi process base class
    """

    def __init__(self: any, sample_config: dict) -> None:
        multiprocessing.Process.__init__(self)
        self.sample_config = sample_config

    def process_init(self: any) -> None:
        """
        init process
        """
        init_log(self.sample_config.get("result_dir"))
        LoadInfoManager.load_info(self.sample_config.get("result_dir"))

    @abstractmethod
    def ms_run(self: any) -> None:
        """
        subclass need to implement this method
        """

    def run(self: any) -> None:
        """
        implement run method
        """
        start_time = time.time()
        self.process_init()
        try:
            self.ms_run()
        except Exception as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        logging.info(f'{self.__class__.__name__} process data finished, '
                     f'execute time is {(time.time() - start_time):.3f}s')
