"""
function: analysis the data of ai cpu by iteration.
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""

import json
import logging
import os
import sqlite3

from common_func.ai_stack_data_check_manager import AiStackDataCheckManager
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import MsvpCommonConst
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from common_func.batch_counter import BatchCounter
from common_func.iter_recorder import IterRecorder
from common_func.utils import Utils
from framework.offset_calculator import OffsetCalculator
from msmodel.ai_cpu.ai_cpu_model import AiCpuModel
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.ai_cpu_data import AiCpuData
from analyzer.scene_base.profiling_scene import ProfilingScene


class ParseAiCpuBinData(MsMultiProcess):
    """
    parse ai cpu data by dp channel
    """
    TAG_AICPU = "AICPU"
    LIMIT_AI_CPU_LEN = 5000
    TABLES_PATH = os.path.join(MsvpCommonConst.CONFIG_PATH, 'Tables.ini')
    AI_CPU_DATA_MAP = "AiCpuDataMap"
    # magic number: 5A5A
    AI_CPU_MAGIC_NUM = 23130
    AI_CPU_TAG = 9
    NONE_NODE_NAME = ''

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self._file_list = file_list.get(DataTag.AI_CPU, [])
        self.sample_config = sample_config
        self.project_path = self.sample_config.get("result_dir")
        self.ai_cpu_datas = []
        self._model = AiCpuModel(self.project_path, [DBNameConstant.TABLE_AI_CPU])
        self._batch_counter = BatchCounter(self.project_path)
        self._batch_counter.init(Constant.TASK_TYPE_AI_CPU)
        self._iter_recorder = IterRecorder(self.project_path)
        self._overstep_task_cnt = 0

    def read_binary_data(self: any, file_path: str) -> None:
        """
        read lines from files
        :param file_path: file path of ai cpu
        :return: NA
        """
        ai_cpu_file = file_path
        file_size = os.path.getsize(ai_cpu_file)
        struct_nums = file_size // StructFmt.AI_CPU_FMT_SIZE
        offset_calculator = OffsetCalculator(self._file_list, StructFmt.AI_CPU_FMT_SIZE,
                                             self.project_path)
        with open(ai_cpu_file, 'rb') as cpu_f:
            cpu_f = offset_calculator.pre_process(cpu_f, file_size)
        for index in range(struct_nums):
            ai_cpu = AiCpuData().ai_cpu_decode(
                cpu_f[index * StructFmt.AI_CPU_FMT_SIZE: (index + 1) * StructFmt.AI_CPU_FMT_SIZE])
            if ai_cpu.ai_cpu_time_consuming.ai_cpu_task_start_time != 0 and \
                    ai_cpu.ai_cpu_time_consuming.ai_cpu_task_end_time:
                if self._iter_recorder.check_task_in_iteration(ai_cpu.ai_cpu_time_consuming.ai_cpu_task_end_syscnt):
                    self.ai_cpu_datas.append(
                        [ai_cpu.stream_id,
                         ai_cpu.task_id,
                         ai_cpu.ai_cpu_time_consuming.ai_cpu_task_start_time,
                         ai_cpu.ai_cpu_time_consuming.ai_cpu_task_end_time,
                         self.NONE_NODE_NAME,
                         ai_cpu.ai_cpu_time_consuming.compute_time,
                         ai_cpu.ai_cpu_time_consuming.memory_copy_time,
                         ai_cpu.ai_cpu_time_consuming.ai_cpu_task_time,
                         ai_cpu.ai_cpu_time_consuming.dispatch_time,
                         ai_cpu.ai_cpu_time_consuming.total_time,
                         self.__calculate_batch_id(
                             ai_cpu.stream_id, ai_cpu.task_id,
                             ai_cpu.ai_cpu_time_consuming.ai_cpu_task_end_syscnt)])
                else:
                    self._overstep_task_cnt = self._overstep_task_cnt + 1
            struct_nums -= 1
            logging.debug(json.dumps(ai_cpu, default=lambda message: message.__dict__, sort_keys=True))

    def parse_ai_cpu(self: any) -> None:
        """
        parse ai cpu
        """
        if not AiStackDataCheckManager.contain_dp_aicpu_data(self.project_path):
            return

        data_dir = PathManager.get_data_dir(self.project_path)
        self._file_list.sort()
        for _file in self._file_list:
            if is_valid_original_data(_file, self.project_path):
                logging.info("start parsing ai cpu data file: %s", _file)
                self.read_binary_data(os.path.join(data_dir, _file))
                FileManager.add_complete_file(self.project_path, _file)
        if self._overstep_task_cnt > 0:
            logging.warning("AI_CPU overstep task number is %s", self._overstep_task_cnt)

    def save(self: any) -> None:
        """
        save data to db
        :return:
        """
        if self._model and self.ai_cpu_datas:
            self._model.init()
            self._model.flush(self.ai_cpu_datas)
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        main entry for parsing ai cpu data
        :return: None
        """
        try:
            self.parse_ai_cpu()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as task_rec_err:
            logging.error(str(task_rec_err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        try:
            self.save()
        except sqlite3.Error as task_rec_err:
            logging.error(str(task_rec_err), exc_info=Constant.TRACE_BACK_SWITCH)

    def __calculate_batch_id(self: any, stream_id: int, task_id: int, syscnt: int) -> int:
        if ProfilingScene().is_operator():
            batch_id = self._batch_counter.calculate_batch(stream_id, task_id)
        else:
            self._iter_recorder.set_current_iter_id(syscnt)
            batch_id = self._batch_counter.calculate_batch(stream_id, task_id, self._iter_recorder.current_iter_id)
        return batch_id
