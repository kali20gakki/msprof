#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.

import multiprocessing
from abc import abstractmethod

from common_func.common import init_log
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
        self.process_init()
        self.ms_run()
