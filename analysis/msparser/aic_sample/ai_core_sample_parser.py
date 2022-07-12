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
from msmodel.aic.ai_core_sample_model import AiCoreSampleModel
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
        self.result_dir = self.sample_config.get("result_dir")
        self.file_list = []
        self.ai_core_data = []
        self.device_id = self.sample_config.get("device_id", "0")
        self.model = None
        self._db_name = 'aicore_{}.db'.format(self.device_id)
        self._replayid = 0
        self._event = self.sample_config.get("ai_core_profiling_events", "").split(",")
        self._sample_metrics_key = self.sample_config.get(StrConstant.AI_CORE_PROFILING_METRICS)

    def get_aicore_type(self: any) -> str:
        """
        get ai core type
        :return:
        """
        return self.sample_config.get('ai_core_profiling_mode')

    def ms_run(self: any) -> None:
        """
        ai core sample based parser
        :return:
        """
        if not self.get_aicore_type() == 'sample-based':
            return
        self.start_parsing_data_file()
        try:
            self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def start_parsing_data_file(self: any) -> None:
        """
        parser ai core / aiv sample based file
        :return:
        """
        try:
            for file_name in self.file_list:
                if is_valid_original_data(file_name, self.result_dir):
                    logging.info(
                        "start parsing data file: %s", file_name)
                    self.read_binary_data(file_name)
                    FileManager.add_complete_file(self.result_dir, file_name)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)

    def save(self: any) -> None:
        """
        save ai core / aiv sample based data to db
        :return:
        """
        self.model = AiCoreSampleModel(self.result_dir, self._db_name, [DBNameConstant.TABLE_EVENT_COUNT])
        if self.model and self.ai_core_data:
            self.model.init()
            self.model.create_core_table(self._event, self.ai_core_data)

            self.model.insert_metric_value()
            freq = InfoConfReader().get_freq(StrConstant.AIV)

            self.model.insert_metric_summary_table(freq, self._sample_metrics_key)
            self.model.finalize()

    def read_binary_data(self: any, binary_data_path: str) -> None:
        """
        read binary data
        :param binary_data_path: file path
        :return:
        """
        if not os.path.exists(PathManager.get_data_file_path(self.result_dir, binary_data_path)):
            return
        file_size = os.path.getsize(PathManager.get_data_file_path(self.result_dir, binary_data_path))
        file_name = PathManager.get_data_file_path(self.result_dir, binary_data_path)
        calculate = OffsetCalculator(self.file_list, StructFmt.AICORE_SAMPLE_FMT_SIZE, self.result_dir)
        try:
            with FileOpen(file_name, 'rb') as file_:
                ai_core_data = calculate.pre_process(file_.file_reader, file_size)
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
                tmp = [aicore_data_bean.count_num, aicore_data_bean.mode, self._replayid,
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
        self.file_list = sorted(file_list.get(DataTag.AI_CORE, []), key=lambda x: int(x.split("_")[-1]))


class ParsingAIVectorCoreSampleData(ParsingCoreSampleData):
    """
    parsing ai vector core data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        ParsingCoreSampleData.__init__(self, sample_config)
        self.file_list = sorted(file_list.get(DataTag.AIV, []), key=lambda x: int(x.split("_")[-1]))
        self._db_name = 'ai_vector_core_{}.db'.format(self.device_id)
        self._event = self.sample_config.get("aiv_profiling_events", "").split(",")
        self._sample_metrics_key = self.sample_config.get(StrConstant.AI_VECTOR_CORE_PROFILING_METRICS)

    def get_aicore_type(self: any) -> str:
        """
        get ai core type
        :return:
        """
        return self.sample_config.get('aiv_profiling_mode')


class ParsingFftsAICoreSampleData(ParsingCoreSampleData):
    """
    parsing ai vector sample data class
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        ParsingCoreSampleData.__init__(self, sample_config)
        self.file_list = sorted(file_list.get(DataTag.FFTS_PMU, []), key=lambda x: int(x.split("_")[-1]))
