#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

import logging
import sqlite3
from typing import List

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from msmodel.add_info.mc2_comm_info_model import Mc2CommInfoViewModel, Mc2CommInfoModel
from msmodel.compact_info.capture_stream_info_model import CaptureStreamInfoModel
from msparser.compact_info.capture_stream_info_bean import CaptureStreamInfoBean
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag


class CaptureStreamInfoParser(DataParser, MsMultiProcess):
    """
    capture stream data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._capture_stream_data = []
        self._capture_mc2_comm_info_data = []

    @staticmethod
    def _get_capture_stream_info_data(bean_data: CaptureStreamInfoBean) -> CaptureStreamInfoBean:
        return bean_data

    def parse(self: any) -> None:
        """
        parse captrue stream data
        """
        stream_info_file = self._file_list.get(DataTag.CAPTURE_STREAM_INFO)
        capture_stream_data = self.parse_bean_data(stream_info_file,
                                                   StructFmt.CAPTURE_STREAM_INFO_SIZE,
                                                   CaptureStreamInfoBean,
                                                   format_func=self._get_capture_stream_info_data)
        self._format_stream_data(capture_stream_data)

    def format_data(self: any) -> None:
        self._format_mc2_comm_info_data()

    def save(self: any) -> None:
        """
        save data
        """
        if self._capture_stream_data:
            with CaptureStreamInfoModel(self._project_path) as _model:
                _model.flush(self._capture_stream_data)

        if self._capture_mc2_comm_info_data:
            with Mc2CommInfoModel(self._project_path) as _model:
                _model.flush(self._capture_mc2_comm_info_data)

    def ms_run(self: any) -> None:
        """
        parse and save capture stream info
        :return:
        """
        if not self._file_list.get(DataTag.CAPTURE_STREAM_INFO, []):
            return
        try:
            self.parse()
            self.format_data()
            self.save()
        except Exception as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _format_mc2_comm_info_data(self):
        comm_info_view_model = Mc2CommInfoViewModel(self._project_path, [DBNameConstant.TABLE_MC2_COMM_INFO])
        if not comm_info_view_model.check_db() or not comm_info_view_model.check_table():
            return

        # 当前先不按device_id分组 后续mc2数据修改后再一起修改
        stream_info_dict = dict()
        for stream_info in self._capture_stream_data:
            # 0 device_id, 1 model_id, 2 original_stream_id, 3 model_stream_id
            stream_info_dict.setdefault(stream_info[2], set()).add(stream_info[3])

        comm_info_data_list = comm_info_view_model.get_kfc_stream(DBNameConstant.TABLE_MC2_COMM_INFO)
        for comm_info_data in comm_info_data_list:
            if comm_info_data.aicpu_kfc_stream_id not in stream_info_dict.keys():
                continue
            # 把capture场景的内容手动补充数据上去 避免后续多处修改
            for models_stream_id in stream_info_dict[comm_info_data.aicpu_kfc_stream_id]:
                self._capture_mc2_comm_info_data.append([comm_info_data.group_name, comm_info_data.rank_size,
                                                         comm_info_data.rank_id, comm_info_data.usr_rank_id,
                                                         models_stream_id, comm_info_data.comm_stream_ids])

    def _format_stream_data(self, capture_stream_data: List[CaptureStreamInfoBean]):
        start_model_set = set()
        end_model_set = set()
        stream_info_set = set()
        repeated_num = 0
        for bean_data in capture_stream_data:
            key = (bean_data.device_id, bean_data.model_id, bean_data.original_stream_id, bean_data.model_stream_id)
            if key in stream_info_set:
                repeated_num += 1
                continue

            # 0 start; 1 end: 记录无需落盘
            if bean_data.act == 0 and bean_data.model_id not in start_model_set:
                start_model_set.add(bean_data.model_id)
            if bean_data.act == 1 and bean_data.model_id not in end_model_set:
                start_model_set.add(bean_data.model_id)
                continue
            stream_info_set.add(key)

        if start_model_set != end_model_set:
            logging.error(f"start model ids are {start_model_set}, end model ids are {end_model_set}.")
        if repeated_num:
            logging.warning(f"There are {repeated_num} duplicate records in total.")

        self._capture_stream_data = [list(t) for t in stream_info_set]
