#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import logging
import sqlite3

from common_func.db_name_constant import DBNameConstant
from common_func.ms_multi_process import MsMultiProcess
from msmodel.ai_cpu.ai_cpu_model import AiCpuModel
from msparser.add_info.aicpu_add_info_bean import AicpuAddInfoBean
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag


class AicpuAddInfoParser(DataParser, MsMultiProcess):
    """
    aicpu data parser
    """
    NONE_NODE_NAME = ''

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)
        self._file_list = file_list
        self.sample_config = sample_config
        self.project_path = self.sample_config.get("result_dir")
        self._ai_cpu_datas = []
        self._model = AiCpuModel(self.project_path, [DBNameConstant.TABLE_AI_CPU])
        self._overstep_task_cnt = 0

    @staticmethod
    def _get_aicpu_info_data(bean_data: any) -> list:
        if bean_data and bean_data.ai_cpu_task_start_time != 0 and bean_data.ai_cpu_task_end_time:
            return [bean_data.stream_id,
                    bean_data.task_id,
                    bean_data.ai_cpu_task_start_time,
                    bean_data.ai_cpu_task_end_time,
                    self.NONE_NODE_NAME,
                    bean_data.compute_time,
                    bean_data.memory_copy_time,
                    bean_data.ai_cpu_task_time,
                    bean_data.dispatch_time,
                    bean_data.total_time]
        return []

    def parse(self: any) -> None:
        """
        parse ai cpu
        """
        aicpu_info_files = self._file_list.get(DataTag.AICPU_ADD_INFO, [])
        for file_list in aicpu_info_files:
            self._ai_cpu_datas.extend(self.parse_bean_data(file_list, StructFmt.NEW_AI_CPU_FMT_SIZE,
                                                                  AicpuAddInfoBean,
                                                                  format_func=self._get_aicpu_info_data,
                                                                  check_func=self.check_magic_num,
                                                                  ))

    def save(self: any) -> None:
        """
        save data to db
        :return:
        """
        if self._model and self._ai_cpu_datas:
            self._model.init()
            self._model.flush(self._ai_cpu_datas)
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        parse and save ge fusion data
        :return:
        """
        if not self._file_list.get(DataTag.AICPU_ADD_INFO, []):
            return
        logging.info("start parsing aicpu data, files: %s",
                         str(self._file_list.get(DataTag.AICPU_ADD_INFO)))
        self.parse()
        self.save()
