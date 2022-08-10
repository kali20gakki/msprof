"""
function: analysis the data of ai cpu by iteration.
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
from msmodel.ai_cpu.ai_cpu_model import AiCpuModel
from msmodel.step_trace.ts_track_model import TsTrackModel
from analyzer.scene_base.profiling_scene import ProfilingScene


class AICpuFromTsParser(MsMultiProcess):
    """
    parse ai cpu from ts
    """
    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._iter_recorder = IterRecorder(self._project_path)
        self._ts_model = TsTrackModel(self._project_path,
                                   DBNameConstant.DB_STEP_TRACE,
                                   [DBNameConstant.TABLE_TASK_TYPE])
        self._aicpu_model = AiCpuModel(self._project_path, [DBNameConstant.TABLE_AI_CPU_FROM_TS])
        self._batch_counter = BatchCounter(self._project_path)
        self._batch_counter.init(Constant.TASK_TYPE_AI_CPU)

    def ms_run(self: any) -> None:
        """
        get ai cpu from ts and save to db
        :return:
        """
        ai_cpu_data = self.process_ai_cpu_data()
        self.save(ai_cpu_data)

    def process_ai_cpu_data(self: any) -> list:
        """
        process ai cpu data
        """

        with self._ts_model:
            ai_cpu_data = self._ts_model.get_ai_cpu_data()

        ai_cpu_data_processed = []
        for stream_id, task_id, sys_start, sys_end in ai_cpu_data:
            ai_cpu_data_processed.append([
                stream_id,
                task_id,
                InfoConfReader().time_from_syscnt(sys_start, NumberConstant.MILLI_SECOND),
                InfoConfReader().time_from_syscnt(sys_end, NumberConstant.MILLI_SECOND),
                self.calculate_batch_id(stream_id, task_id, sys_end)
            ])
        return ai_cpu_data_processed

    def calculate_batch_id(self: any, stream_id: int, task_id: int, syscnt: int) -> int:
        if ProfilingScene().is_operator():
            batch_id = self._batch_counter.calculate_batch(stream_id, task_id)
        else:
            self._iter_recorder.set_current_iter_id(syscnt)
            batch_id = self._batch_counter.calculate_batch(stream_id, task_id, self._iter_recorder.current_iter_id)
        return batch_id

    def save(self: any, ai_cpu_data: list) -> None:
        """
        save ai cpu data
        """
        with self._aicpu_model:
            self._aicpu_model.flush(ai_cpu_data, DBNameConstant.TABLE_AI_CPU_FROM_TS)
