#!/usr/bin/python3
# coding=utf-8
"""
function: analysis the data of ai cpu from ts by iteration.
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""

from common_func.ms_multi_process import MsMultiProcess
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.db_name_constant import DBNameConstant
from common_func.iter_recorder import IterRecorder
from common_func.batch_counter import BatchCounter
from common_func.info_conf_reader import InfoConfReader
from common_func.constant import Constant
from msmodel.step_trace.ts_track_model import TsTrackModel
from mscalculate.ts_task.task_state_handler import TaskStateHandler
from mscalculate.ts_task.ai_cpu.aicpu_from_ts_collector import AICpuFromTsCollector
from analyzer.scene_base.profiling_scene import ProfilingScene


class AICpuFromTsCalculator(MsMultiProcess):
    """
    parse ai cpu from ts
    """
    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._ts_model = TsTrackModel(self._project_path,
                                   DBNameConstant.DB_STEP_TRACE,
                                   [DBNameConstant.TABLE_TASK_TYPE])
        self._aicpu_collector = AICpuFromTsCollector(self._project_path)
        self._iter_recorder = IterRecorder(self._project_path)
        self._batch_counter = BatchCounter(self._project_path)
        self._batch_counter.init(Constant.TASK_TYPE_AI_CPU)

    @staticmethod
    def state_to_timeline(ai_cpu_with_state: list) -> list:
        """
        transfer state to start and end
        :param ai_cpu_with_state: ai cpu data with start and end
        :return: ai cpu timeline list
        """
        stream_task_group = {}
        for stream_id, task_id, timestamp, task_state in ai_cpu_with_state:
            task_state_handler = stream_task_group.setdefault(
                (stream_id, task_id), TaskStateHandler(stream_id, task_id))
            task_state_handler.process_record(timestamp, task_state)

        aicpu_timeline_list = []
        for task_state_handler in stream_task_group.values():
            aicpu_timeline_list.extend(task_state_handler.task_timeline_list)
        aicpu_timeline_list.sort(key=lambda task_timeline: task_timeline.end)
        return aicpu_timeline_list

    def ms_run(self: any) -> None:
        """
        get ai cpu from ts and save to db
        :return:
        """
        self.process_ai_cpu_data()
        self._aicpu_collector.save_aicpu()

    def process_ai_cpu_data(self: any):
        """
        process ai cpu data
        """
        with self._ts_model:
            ai_cpu_with_state = self._ts_model.get_ai_cpu_data(
                self.sample_config.get("model_id"), self.sample_config.get("iter_id"))

        aicpu_timeline_list = self.state_to_timeline(ai_cpu_with_state)

        aicpu_list = []
        for aicpu_timeline in aicpu_timeline_list:
            aicpu_list.append([aicpu_timeline.stream_id,
                               aicpu_timeline.task_id,
                               InfoConfReader().time_from_syscnt(aicpu_timeline.start,
                               NumberConstant.MILLI_SECOND),
                               InfoConfReader().time_from_syscnt(aicpu_timeline.end,
                               NumberConstant.MILLI_SECOND),
                               self.calculate_batch_id(aicpu_timeline.stream_id,
                               aicpu_timeline.task_id, aicpu_timeline.end)])

        self._aicpu_collector.aicpu_list = aicpu_list

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
