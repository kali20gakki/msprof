#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import json
import logging
import os
from json import JSONDecodeError

from common_func.common import error
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from msmodel.parallel.parallel_model import ParallelModel
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag


class ParallelStrategyParser(IParser, MsMultiProcess):
    FILE_MAX_SIZE = 9999999
    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any, file_list: dict, sample_config: dict):
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._parallel_strategy_data = []

    def ms_run(self) -> None:
        parallel_files = self._file_list.get(DataTag.PARALLEL_STRATEGY, [])
        if not parallel_files:
            return
        logging.info("Start to parse parallel strategy data!")
        self.parse(parallel_files)
        self.save()

    def parse(self: any, parallel_files: list) -> None:
        parallel_data = ""
        for _parallel_file in parallel_files:
            parallel_file = PathManager.get_data_file_path(self._project_path, _parallel_file)
            _file_size = os.path.getsize(parallel_file)
            if _file_size < 0 or _file_size > self.FILE_MAX_SIZE:
                error(self.FILE_NAME, "Parallel strategy file size exceeds the upper limit.")
                return
            with FileOpen(parallel_file, 'rt') as _file:
                parallel_data = parallel_data + _file.file_reader.read()
        try:
            parallel_data = json.loads(parallel_data).get("config", {})
        except JSONDecodeError:
            error(self.FILE_NAME, "Invalid parallel strategy data.")
            return
        if not parallel_data:
            error(self.FILE_NAME, "Invalid parallel strategy data.")
            return
        parallel_mode = self._get_parallel_mode(parallel_data.get("parallelType"), parallel_data.get("stage_num"))
        self._parallel_strategy_data.append([parallel_data.get("ai_framework_type"), parallel_data.get("stage_num"),
                                             parallel_data.get("rankId"), parallel_data.get("stageId"),
                                             parallel_data.get("parallelType"), str(parallel_data.get("stageDevices")),
                                             parallel_mode])

    def save(self: any) -> None:
        if not self._parallel_strategy_data:
            return
        with ParallelModel(self._project_path) as _model:
            _model.flush(DBNameConstant.TABLE_PARALLEL_STRATEGY, self._parallel_strategy_data)

    def _get_parallel_mode(self: any, parallel_type: str, stage_num: int) -> str:
        parallel_type = StrConstant.DATA_PARALLEL if not parallel_type else parallel_type
        stage_num = 1 if not stage_num else stage_num
        if stage_num > 1:
            return StrConstant.PIPELINE_PARALLEL
        return StrConstant.MODEL_PARALLEL if parallel_type != StrConstant.DATA_PARALLEL else parallel_type
