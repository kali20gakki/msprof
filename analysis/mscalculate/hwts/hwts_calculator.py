#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to calculate hwts offset and parse it.
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""
import logging
import os

from analyzer.scene_base.profiling_scene import ProfilingScene
from analyzer.op_common_function import OpCommonFunc
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.constant import Constant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_iteration import MsprofIteration
from common_func.path_manager import PathManager
from common_func.utils import Utils
from common_func.batch_counter import BatchCounter
from framework.offset_calculator import FileCalculator
from framework.offset_calculator import OffsetCalculator
from model.iter_rec.iter_rec_model import HwtsIterModel
from model.task_time.hwts_log_model import HwtsLogModel
from mscalculate.interface.icalculator import ICalculator
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.hwts_log import HwtsLogBean


class HwtsCalculator(ICalculator, MsMultiProcess):
    """
    class used to calculate hwts offset and parse log by iter
    """
    # Tags for differnt HWTS log type.
    HWTS_TASK_START = 0
    HWTS_TASK_END = 1
    HWTS_LOG_SIZE = 64
    HWTS_MAX_CNT = 16
    TASK_TIME_COMPLETE_INDEX = 3

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._file_list = file_list.get(DataTag.HWTS, [])
        self._hwts_log_model = HwtsLogModel(self._project_path)
        self._iter_model = HwtsIterModel(self._project_path)
        self._log_data = []
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    def calculate(self: any) -> None:
        """
        calculate hwts data
        :return: None
        """
        if ProfilingScene().is_operator():
            self._parse_all_file()
        else:
            self._parse_by_iter()

    def save(self: any) -> None:
        """
        save hwts data
        :return: None
        """
        self._hwts_log_model.clear()
        if self._log_data:
            self._hwts_log_model.init()
            self._hwts_log_model.flush(Utils.obj_list_to_list(self._log_data), DBNameConstant.TABLE_HWTS_TASK)
            self._hwts_log_model.flush(self._add_batch_id(self._prep_data()), DBNameConstant.TABLE_HWTS_TASK_TIME)
            self._hwts_log_model.finalize()

    def ms_run(self: any) -> None:
        """
        entrance for calculating hwts
        :return: None
        """
        if self._file_list:
            self.calculate()
            self.save()

    def _prep_data(self: any) -> list:
        """
        prepare data for tasktime table
        :return:
        """
        prep = {}
        for task in self._log_data:
            index = ",".join(map(str, [task.stream_id, task.task_id]))
            task_value = prep.get(index, {})
            task_satrt_list = task_value.get(self.HWTS_TASK_START, [])
            task_end_list = task_value.get(self.HWTS_TASK_END, [])
            if self.HWTS_TASK_START == task.task_type:
                if len(task_satrt_list) > len(task_end_list):
                    task_satrt_list.pop()
                    logging.warning("stream id: %s, task id: %s is no end task, index of the stream and task is %s",
                                    task.stream_id, task.task_id, str(len(task_satrt_list)))
                task_satrt_list.append(task.sys_cnt)
            elif self.HWTS_TASK_END == task.task_type:
                if len(task_satrt_list) == len(task_end_list):
                    continue
                task_end_list.append(task.sys_cnt)
            else:
                logging.error("invalid task type of hwts: %s", task.task_type)
            task_value.update({self.HWTS_TASK_START: task_satrt_list, self.HWTS_TASK_END: task_end_list})
            prep.update({index: task_value})
        train_data = []
        for index in prep:
            _index_value = prep.get(index)
            while _index_value.get(self.HWTS_TASK_START) and _index_value.get(self.HWTS_TASK_END):
                train_data.append(tuple(list(index.split(",")) + [
                    InfoConfReader().time_from_syscnt(_index_value[self.HWTS_TASK_START].pop(0)),
                    InfoConfReader().time_from_syscnt(_index_value[self.HWTS_TASK_END].pop(0)),
                    self._sample_config.get('iter_id'), self._sample_config.get('model_id')]))
        return sorted(train_data, key=lambda data: data[self.TASK_TIME_COMPLETE_INDEX])

    def _parse_by_iter(self: any) -> None:
        """
        Parse the specified iteration data
        :return: None
        """
        if self._iter_model.check_db() and self._iter_model.check_table():
            _iter_id = MsprofIteration(self._project_path). \
                get_iteration_id_by_index_id(self._sample_config.get("iter_id"), self._sample_config.get("model_id"))

            offset_count, total_count = self._iter_model.get_task_offset_and_sum(_iter_id, 'task_count')
            if not total_count:
                return
            _file_calculator = FileCalculator(self._file_list, self.HWTS_LOG_SIZE, self._project_path,
                                              offset_count, total_count)
            self._parse(_file_calculator.prepare_process())
            self._iter_model.finalize()

    def _parse_all_file(self: any) -> None:
        _offset_calculator = OffsetCalculator(self._file_list, self.HWTS_LOG_SIZE, self._project_path)
        for _file in self._file_list:
            _file = PathManager.get_data_file_path(self._project_path, _file)
            with open(_file, 'rb') as _hwts_log_reader:
                self._parse(_offset_calculator.pre_process(_hwts_log_reader, os.path.getsize(_file)))

    def _add_batch_id(self: any, prep_data_res: list) -> list:
        if ProfilingScene().is_operator():
            batch_counter = BatchCounter(self._project_path)
            batch_counter.init(Constant.TASK_TYPE_AI_CORE)
            for index, datum in enumerate(prep_data_res):
                # index 0 stream id, index 1 task id
                batch_id = batch_counter.calculate_batch(datum[0], datum[1])
                prep_data_res[index] = list(datum) + [batch_id]
        else:
            self._iter_model.init()
            _iter_id = MsprofIteration(self._project_path). \
                get_iteration_id_by_index_id(self._sample_config.get("iter_id"), self._sample_config.get("model_id"))
            batch_list = self._iter_model.get_batch_list(_iter_id, DBNameConstant.TABLE_HWTS_BATCH)

            if len(batch_list) != len(prep_data_res):
                logging.warning("hwts data can not match with batch id list.")
                return []

            for index, batch in enumerate(batch_list):
                # type of batch is tuple
                prep_data_res[index] = prep_data_res[index] + batch
        return prep_data_res

    def _parse(self: any, all_log_bytes: bytes) -> None:
        for log_data in Utils.chunks(all_log_bytes, self.HWTS_LOG_SIZE):
            _task_log = HwtsLogBean.decode(log_data)
            if _task_log.is_log_type():
                self._log_data.append(_task_log)
