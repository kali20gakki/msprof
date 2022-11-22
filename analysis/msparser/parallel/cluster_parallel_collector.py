#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import logging
import os
import threading

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from msmodel.cluster_info.cluster_info_model import ClusterInfoViewModel
from msmodel.parallel.cluster_parallel_model import ClusterParallelModel
from msmodel.parallel.parallel_model import ParallelViewModel
from msparser.interface.iparser import IParser
from profiling_bean.db_dto.cluster_rank_dto import ClusterRankDto


class ClusterParallelCollector(IParser):
    THREAD_NUM = 10

    def __init__(self: any, collect_path: str) -> None:
        self.collect_path = collect_path
        self._cluster_info = []
        self._cluster_parallel_temp_data = []
        self._cluster_parallel_strategy_temp_data = []
        self._parallel_table_name = Constant.NA

    def ms_run(self) -> None:
        self.parse()
        self.save()

    def parse(self: any) -> None:
        with ClusterInfoViewModel(self.collect_path) as _model:
            if _model.check_table():
                self._cluster_info = _model.get_all_cluster_rank_info()
        if not self._cluster_info:
            return

        _project_path = os.path.join(self.collect_path, self._cluster_info[0].dir_name)
        with ParallelViewModel(_project_path) as _model:
            self._parallel_table_name = _model.get_parallel_table_name()
        if self._parallel_table_name == Constant.NA:
            return
        logging.info("Start to parse cluster parallel data!")
        self.multithreading_get_parallel_data()

    def save(self: any) -> None:
        cluster_parallel_data = []
        for parallel_data in self._cluster_parallel_temp_data:
            if parallel_data:
                cluster_parallel_data.extend(parallel_data)
        cluster_parallel_strategy_data = []
        for parallel_strategy_data in self._cluster_parallel_strategy_temp_data:
            if parallel_strategy_data:
                cluster_parallel_strategy_data.extend(parallel_strategy_data)
        if not cluster_parallel_data or not cluster_parallel_strategy_data:
            logging.warning("Invalid cluster parallel data!")
            return
        with ClusterParallelModel(self.collect_path) as _model:
            _model.create_table(self._parallel_table_name)
            _model.flush(self._parallel_table_name, cluster_parallel_data)
            _model.create_table(DBNameConstant.TABLE_CLUSTER_PARALLEL_STRATEGY)
            _model.flush(DBNameConstant.TABLE_CLUSTER_PARALLEL_STRATEGY, cluster_parallel_strategy_data)

    def get_device_parallel_data(self: any, cluster_info: ClusterRankDto, index: int):
        _project_path = os.path.join(self.collect_path, cluster_info.dir_name)
        InfoConfReader().load_info(_project_path)
        freq_to_us = 1000000 / InfoConfReader().get_freq(StrConstant.HWTS)
        with ParallelViewModel(_project_path) as _model:
            self._cluster_parallel_temp_data[index] = _model.get_parallel_index_data(self._parallel_table_name,
                                                                                     cluster_info.rank_id,
                                                                                     cluster_info.device_id, freq_to_us)
            self._cluster_parallel_strategy_temp_data[index] = _model.get_parallel_strategy_data()

    def multithreading_get_parallel_data(self: any) -> None:
        prof_num = len(self._cluster_info)
        self._cluster_parallel_temp_data = [None] * prof_num
        self._cluster_parallel_strategy_temp_data = [None] * prof_num
        index = 0
        for i in range(prof_num // self.THREAD_NUM + 1):
            thread_list = []
            num = self.THREAD_NUM
            if i == prof_num // self.THREAD_NUM and prof_num % self.THREAD_NUM > 0:
                num = prof_num % self.THREAD_NUM
            if i == prof_num // self.THREAD_NUM and prof_num % self.THREAD_NUM == 0:
                num = 0
            for thread_id in range(num):
                thread_list.append(
                    threading.Thread(target=self.get_device_parallel_data, args=(self._cluster_info[index], index)))
                index += 1
            for thread in thread_list:
                thread.start()
            for thread in thread_list:
                thread.join()
