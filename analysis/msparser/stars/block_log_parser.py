#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.

from common_func.ms_constant.stars_constant import StarsConstant
from msmodel.stars.block_log_model import BlockLogModel
from msparser.stars.log_base_parser import LogBaseParser
from profiling_bean.stars.block_log_bean import BlockLogBean


class BlockLogParser(LogBaseParser):
    """
    class used to parser AIC/AIV block log
    """

    def __init__(self: any, result_dir: str, db: str, table_list: list) -> None:
        super().__init__(result_dir)
        self._model = BlockLogModel(result_dir, db, table_list)
        self._decoder = BlockLogBean
        self._data_list = []
        self._mismatch_task = []
        self._start_functype = StarsConstant.BLOCK_LOG_START_FUNCTYPE
        self._end_functype = StarsConstant.BLOCK_LOG_END_FUNCTYPE
