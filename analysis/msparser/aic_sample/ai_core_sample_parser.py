#!/usr/bin/python3
# coding=utf-8
"""
This script is used to create ai core db for sample-based ai core data
Copyright Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
"""

import logging
import os
from abc import abstractmethod

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from framework.offset_calculator import OffsetCalculator
from model.aic.ai_core_sample_model import AiCoreSampleModel
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.aicore_sample import AicoreSample


class ParsingCoreSampleData(MsMultiProcess):
    """
    base parsing sample data class
    """

    def __init__(self: any, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self.sample_config = sample_config
        self.ai_core_data = []
        self.device_id = self.sample_config.get("device_id", "0")
        self.replayid = 0

    @abstractmethod
    def ms_run(self: any) -> None:
        """
        ai core sample based parser
        :return:
        """

    def read_binary_data(self: any, binary_data_path: str) -> None:
        """
        read binary data
        :param binary_data_path: file path
        :return:
        """
        project_path = self.sample_config.get("result_dir")
        if not os.path.exists(PathManager.get_data_file_path(project_path, binary_data_path)):
            return
        file_size = os.path.getsize(PathManager.get_data_file_path(project_path, binary_data_path))
        file_name = PathManager.get_data_file_path(project_path, binary_data_path)
        try:
            with FileOpen(file_name, 'rb') as file_:
                ai_core_data = self.calculate.pre_process(file_.file_reader, file_size)
                self.__insert_ai_core_data(file_size, ai_core_data)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s: %s", binary_data_path, err, exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass

    def __insert_ai_core_data(self: any, file_size: int, ai_core_data: list) -> None:
        delta_dev = InfoConfReader().get_delta_time()
        for _index in range(file_size // StructFmt.AICORE_SAMPLE_FMT_SIZE):
            binary_data = ai_core_data[_index * StructFmt.AICORE_SAMPLE_FMT_SIZE
                                       :(_index + 1) * StructFmt.AICORE_SAMPLE_FMT_SIZE]

            if not binary_data[0]:
                break
            aicore_data_bean = AicoreSample.decode(binary_data)

            if aicore_data_bean is not None:
                tmp = [aicore_data_bean.count_num, aicore_data_bean.mode, self.replayid,
                       aicore_data_bean.timestamp + delta_dev * NumberConstant.NANO_SECOND,
                       aicore_data_bean.core_id, aicore_data_bean.task_cyc]
                tmp.extend(aicore_data_bean.event_count[:aicore_data_bean.count_num])
                self.ai_core_data.append(tmp)
            else:
                break


class ParsingAICoreSampleData(ParsingCoreSampleData):
    """
    parsing ai vector sample data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        ParsingCoreSampleData.__init__(self, sample_config)
        self._file_list = file_list
        self._model = AiCoreSampleModel(sample_config.get('result_dir', ''), 'aicore_{}.db'.format(self.device_id),
                                        [DBNameConstant.TABLE_EVENT_COUNT])
        self.calculate = OffsetCalculator(file_list.get(DataTag.AI_CORE, []), StructFmt.AICORE_SAMPLE_FMT_SIZE,
                                          sample_config.get('result_dir'))

    def start_parsing_data_file(self: any) -> None:
        """
        parsing data file
        :return: None
        """
        project_path = self.sample_config.get("result_dir")
        try:
            for file_name in sorted(self._file_list.get(DataTag.AI_CORE, []), key=lambda x: int(x.split("_")[-1])):
                if is_valid_original_data(file_name, project_path):
                    logging.info(
                        "start parsing aicore data file: %s", file_name)
                    self.read_binary_data(file_name)
                    FileManager.add_complete_file(project_path, file_name)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        logging.info("Create AICore DB finished!")

    def save(self: any) -> None:
        """
        save the ai core data of sample based
        :return: None
        """
        if self._model and self.ai_core_data:
            self._model.init()
            event_ = self.sample_config.get("ai_core_profiling_events", "").split(",")
            self._model.create_ai_vector_core_db(event_, self.ai_core_data)
            sample_metrics_key = self.sample_config.get(StrConstant.AI_CORE_PROFILING_METRICS)
            self._model.insert_metric_value()
            freq = InfoConfReader().get_freq(StrConstant.AIC)
            self._model.insert_metric_summary_table(freq, sample_metrics_key)
            self._model.finalize()

    def ms_run(self: any) -> None:
        if not self.sample_config.get('ai_core_profiling_mode') == 'sample-based':
            return
        self.start_parsing_data_file()
        try:
            self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)


class ParsingAIVectorCoreSampleData(ParsingCoreSampleData):
    """
    parsing ai vector core data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        ParsingCoreSampleData.__init__(self, sample_config)
        self._file_list = file_list
        self._model = AiCoreSampleModel(sample_config.get('result_dir', ''),
                                        'ai_vector_core_{}.db'.format(self.device_id),
                                        [DBNameConstant.TABLE_EVENT_COUNT])
        self.calculate = OffsetCalculator(file_list.get(DataTag.AIV, []), StructFmt.AICORE_SAMPLE_FMT_SIZE,
                                          sample_config.get('result_dir'))

    def start_parsing_data_file(self: any) -> None:
        """
        parsing data file
        :return: None
        """
        project_path = self.sample_config.get("result_dir")
        try:
            for file_name in sorted(self._file_list.get(DataTag.AIV, []), key=lambda x: int(x.split("_")[-1])):
                if is_valid_original_data(file_name, project_path):
                    logging.info(
                        "start parsing ai vector core data file: %s", file_name)
                    self.read_binary_data(file_name)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        logging.info("Create AIVectorCore DB finished!")

    def save(self: any) -> None:
        """
        save the ai vector core data of sample based
        :return: None
        """
        if self._model and self.ai_core_data:
            self._model.init()
            event_ = self.sample_config.get("aiv_profiling_events", "").split(",")
            self._model.create_ai_vector_core_db(event_, self.ai_core_data)
            sample_metrics_key = self.sample_config.get(StrConstant.AI_CORE_PROFILING_METRICS)
            self._model.insert_metric_value()
            freq = InfoConfReader().get_freq(StrConstant.AIV)
            self._model.insert_metric_summary_table(freq, sample_metrics_key)
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        entrance for ai core parser of the sample based
        :return: None
        """
        if not self.sample_config.get('aiv_profiling_mode') == 'sample-based':
            return
        self.start_parsing_data_file()
        try:
            self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
