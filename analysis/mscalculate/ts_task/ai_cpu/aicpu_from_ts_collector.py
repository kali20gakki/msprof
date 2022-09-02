#!/usr/bin/python3
# coding=utf-8
"""
function: collect the data of ai cpu by iteration.
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
from analyzer.scene_base.profiling_scene import ProfilingScene
from msmodel.ai_cpu.ai_cpu_model import AiCpuModel
from common_func.batch_counter import BatchCounter
from common_func.db_name_constant import DBNameConstant
from common_func.constant import Constant
from common_func.iter_recorder import IterRecorder
from common_func.ms_constant.number_constant import NumberConstant
from common_func.platform.chip_manager import ChipManager
from common_func.info_conf_reader import InfoConfReader


class AICpuFromTsCollector:
    """
    AI Cpu from ts collector
    """
    STREAM_TASK_KEY_FMT = "{0}-{1}"
    STREAM_TASK_BATCH_KEY_FMT = "{0}-{1}-{2}"
    AI_CPU_TYPE = 1

    def __init__(self: any, project_path: str) -> None:
        self._project_path = project_path
        self._aicpu_model = AiCpuModel(self._project_path, [DBNameConstant.TABLE_AI_CPU_FROM_TS])
        self._batch_counter = BatchCounter(self._project_path)
        self._batch_counter.init(Constant.TASK_TYPE_AI_CPU)
        self._iter_recorder = IterRecorder(self._project_path)
        self.aicpu_list = []

    def filter_aicpu(self: any, aicpu_feature: tuple) -> None:
        """
        filter ai cpu
        :param aicpu_feature: aicpu feature
        """
        stream_id, task_id, start, end, task_type = aicpu_feature
        if task_type == self.AI_CPU_TYPE:
            batch_id = NumberConstant.DEFAULT_BATCH_ID
            start_ms = InfoConfReader().time_from_syscnt(start)
            end_ms = InfoConfReader().time_from_syscnt(end)
            if not ChipManager().is_chip_v1():
                batch_id = self.calculate_batch_id(stream_id, task_id, end)
                start_ms = InfoConfReader().time_from_syscnt(start) / NumberConstant.MS_TO_NS
                end_ms = InfoConfReader().time_from_syscnt(end) / NumberConstant.MS_TO_NS

            self.aicpu_list.append([int(stream_id),
                                    int(task_id),
                                    start_ms,
                                    end_ms,
                                    batch_id])

    def save_aicpu(self: any) -> None:
        """
        save ai cpu data
        """
        with self._aicpu_model:
            self._aicpu_model.flush(self.aicpu_list, DBNameConstant.TABLE_AI_CPU_FROM_TS)

    def calculate_batch_id(self: any, stream_id: int, task_id: int, syscnt: int) -> int:
        """
        calculate batch id
        :param stream_id: stream id
        :param task_id: task id
        :param syscnt: syscnt
        :return: batch id
        """
        if ProfilingScene().is_operator():
            batch_id = self._batch_counter.calculate_batch(stream_id, task_id)
        else:
            self._iter_recorder.set_current_iter_id(syscnt)
            batch_id = self._batch_counter.calculate_batch(stream_id, task_id, self._iter_recorder.current_iter_id)
        return batch_id
