#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import logging
from typing import List

from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from msparser.compact_info.task_track_bean import TaskTrackBean
from msmodel.compact_info.task_track_model import TaskTrackModel
from msmodel.ge.ge_hash_model import GeHashViewModel
from profiling_bean.prof_enum.data_tag import DataTag


class TaskTrackParser(DataParser, MsMultiProcess):
    """
    task track data parser
    """
    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._task_track_data = []

    def reformat_data(self: any, bean_data: List[TaskTrackBean]) -> List[List]:
        """
        transform bean to data
        """
        with GeHashViewModel(self._project_path) as _model:
            hash_dict = _model.get_type_hash_data()
        return [
            [
                bean.device_id,
                bean.timestamp,
                hash_dict.get(bean.level, {}).get(bean.task_type, bean.task_type),  # task type
                bean.stream_id,
                bean.task_id,
                bean.thread_id,
                bean.batch_id,
                hash_dict.get(bean.level, {}).get(bean.data_type, bean.data_type),  # task track type
                bean.level,
                bean.data_len,
            ] for bean in bean_data
        ]

    def save(self: any) -> None:
        """
        save task track data
        """
        if not self._task_track_data:
            return
        with TaskTrackModel(self._project_path) as model:
            model.flush(self._task_track_data)

    def parse(self: any) -> None:
        """
        parse task track data
        """
        track_files = self._file_list.get(DataTag.TASK_TRACK, [])
        track_files = self.group_aging_file(track_files)
        if not track_files:
            return
        bean_data = []
        for files in track_files.values():
            bean_data += self.parse_bean_data(
                files,
                StructFmt.TASK_TRACK_DATA_SIZE,
                TaskTrackBean,
                lambda x: x,
            )
        self._task_track_data = self.reformat_data(bean_data)

    def ms_run(self: any) -> None:
        """
        parse and save task track data
        :return:
        """
        if not self._file_list.get(DataTag.TASK_TRACK, []):
            return
        logging.info("start parsing task track data, files: %s", str(self._file_list.get(DataTag.TASK_TRACK, [])))
        self.parse()
        self.save()
