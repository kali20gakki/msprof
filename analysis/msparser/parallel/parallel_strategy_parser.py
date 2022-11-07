#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import json
import logging

from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from msmodel.parallel.parallel_model import ParallelModel
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag


class ParallelStrategyParser(IParser, MsMultiProcess):
    def __init__(self: any, file_list: dict, sample_config: dict):
        MsMultiProcess.__init__(self, sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._parallel_strategy_data = []

    def ms_run(self) -> None:
        self.parse()
        self.save()
        logging.info("parallel.db created successful!")

    def parse(self: any) -> None:
        parallel_files = self._file_list.get(DataTag.PARALLEL_STRATEGY, [])
        if not parallel_files:
            return
        logging.info("Start to parse parallel strategy data!")
        parallel_data = ""
        for _parallel_file in parallel_files:
            parallel_file = PathManager.get_data_file_path(self._project_path, _parallel_file)
            with FileOpen(parallel_file, 'rt') as _file:
                parallel_data = parallel_data + _file.file_reader.readline()
        parallel_data = json.loads(parallel_data).get("config", {})
        parallel_mode = self._get_parallel_model(parallel_data.get("parallelType"), parallel_data.get("stage_num"))
        self._parallel_strategy_data.append([parallel_data.get("ai_framework_type"), parallel_data.get("stage_num"),
                                             parallel_data.get("rankId"), parallel_data.get("stageId"),
                                             parallel_data.get("parallelType"), str(parallel_data.get("stageDevices")),
                                             parallel_mode])

    def save(self: any) -> None:
        if not self._parallel_strategy_data:
            logging.error('No valid parallel strategy data.')
            return
        with ParallelModel(self._project_path) as _model:
            _model.flush(DBNameConstant.TABLE_PARALLEL_STRATEGY, self._parallel_strategy_data)

    def _get_parallel_model(self: any, parallelType: str, stage_num: int) -> str:
        parallelType = "data-parallel" if not parallelType else parallelType
        stage_num = 1 if not stage_num else stage_num
        if stage_num > 1:
            parallel_mode = "pipeline-parallel"
        elif parallelType != "data_parallel":
            parallel_mode = "model-parallel"
        else:
            parallel_mode = "data-parallel"
        return parallel_mode
