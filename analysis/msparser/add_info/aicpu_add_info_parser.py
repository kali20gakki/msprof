#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import logging

from common_func.db_name_constant import DBNameConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.ms_constant.str_constant import StrConstant
from common_func.info_conf_reader import InfoConfReader
from msmodel.ai_cpu.ai_cpu_model import AiCpuModel
from msmodel.step_trace.ts_track_model import TsTrackModel
from msmodel.ai_cpu.data_preparation_model import DataPreparationModel
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
        self.project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._aicpu_data = {
            AicpuAddInfoBean.AICPU_NODE: [],
            AicpuAddInfoBean.AICPU_DP: [],
            AicpuAddInfoBean.AICPU_MODEL: [],
            AicpuAddInfoBean.AICPU_MI: [],
        }
        self._get_data_func = {
            AicpuAddInfoBean.AICPU_NODE: self.get_aicpu_node_data,
            AicpuAddInfoBean.AICPU_DP: self.get_aicpu_dp_data,
            AicpuAddInfoBean.AICPU_MODEL: self.get_aicpu_model_data,
            AicpuAddInfoBean.AICPU_MI: self.get_aicpu_mi_data,
        }

    @staticmethod
    def get_aicpu_node_data(aicpu_info: AicpuAddInfoBean) -> list:
        return [
            aicpu_info.data.stream_id,
            aicpu_info.data.task_id,
            aicpu_info.data.ai_cpu_task_start_time,
            aicpu_info.data.ai_cpu_task_end_time,
            AicpuAddInfoParser.NONE_NODE_NAME,
            aicpu_info.data.compute_time,
            aicpu_info.data.memory_copy_time,
            aicpu_info.data.ai_cpu_task_time,
            aicpu_info.data.dispatch_time,
            aicpu_info.data.total_time
        ]

    @staticmethod
    def get_aicpu_dp_data(aicpu_info: AicpuAddInfoBean) -> list:
        return [
            InfoConfReader().trans_into_local_time(float(aicpu_info.timestamp)),
            aicpu_info.data.action,
            aicpu_info.data.source,
            aicpu_info.data.buffer_size,
        ]

    @staticmethod
    def get_aicpu_model_data(aicpu_info: AicpuAddInfoBean) -> list:
        return [
            aicpu_info.data.index_id,
            aicpu_info.data.model_id,
            aicpu_info.timestamp,
            aicpu_info.data.tag_id,
            aicpu_info.data.event_id,
        ]

    @staticmethod
    def get_aicpu_mi_data(aicpu_info: AicpuAddInfoBean) -> list:
        return [
            aicpu_info.data.node_name,
            aicpu_info.data.queue_size,
            aicpu_info.data.start_time,
            aicpu_info.data.end_time,
            aicpu_info.data.duration,
        ]

    def parse(self: any) -> None:
        """
        parse ai cpu
        """
        aicpu_files = self._file_list.get(DataTag.AICPU_ADD_INFO, [])
        aicpu_info = self.parse_bean_data(
            aicpu_files,
            StructFmt.AI_CPU_ADD_FMT_SIZE,
            AicpuAddInfoBean,
            format_func=lambda x: x,
            check_func=self.check_magic_num
        )
        self.set_aicpu_data(aicpu_info)

    def save(self: any) -> None:
        """
        save data to db
        :return:
        """
        aicpu_node_data = self._aicpu_data.get(AicpuAddInfoBean.AICPU_NODE, [])
        aicpu_dp_data = self._aicpu_data.get(AicpuAddInfoBean.AICPU_DP, [])
        aicpu_model_data = self._aicpu_data.get(AicpuAddInfoBean.AICPU_MODEL, [])
        aicpu_mi_data = self._aicpu_data.get(AicpuAddInfoBean.AICPU_MI, [])
        if aicpu_node_data:
            with AiCpuModel(self.project_path, [DBNameConstant.TABLE_AI_CPU]) as model:
                model.flush(aicpu_node_data, DBNameConstant.TABLE_AI_CPU)
        if aicpu_dp_data:
            with AiCpuModel(self.project_path, [DBNameConstant.TABLE_AI_CPU_DP]) as model:
                model.flush(aicpu_dp_data, DBNameConstant.TABLE_AI_CPU_DP)
        if aicpu_model_data:
            with TsTrackModel(self.project_path,
                              DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_MODEL_WITH_Q]) as model:
                model.create_table(DBNameConstant.TABLE_MODEL_WITH_Q)
                model.flush(DBNameConstant.TABLE_MODEL_WITH_Q, aicpu_model_data)
        if aicpu_mi_data:
            with DataPreparationModel(self.project_path, [DBNameConstant.TABLE_DATA_QUEUE]) as model:
                model.flush(aicpu_mi_data)

    def ms_run(self: any) -> None:
        """
        parse and save ge fusion data
        :return:
        """
        if not self._file_list.get(DataTag.AICPU_ADD_INFO, []):
            return
        logging.info("start parsing aicpu data, files: %s", str(self._file_list.get(DataTag.AICPU_ADD_INFO)))
        self.parse()
        self.save()

    def set_aicpu_data(self: any, aicpu_data: list) -> None:
        for aicpu_info in aicpu_data:
            get_data_func = self._get_data_func.get(int(aicpu_info.struct_type))
            if not get_data_func:
                logging.error("The aicpu type %d is invalid.", aicpu_info.struct_type)
                continue
            if int(aicpu_info.struct_type) == AicpuAddInfoBean.AICPU_NODE and \
                    (aicpu_info.data.ai_cpu_task_start_time == 0 or aicpu_info.data.ai_cpu_task_end_time == 0):
                continue
            self._aicpu_data.get(int(aicpu_info.struct_type)).append(get_data_func(aicpu_info))
