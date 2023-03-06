#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import itertools
import logging
import os
import sqlite3
import struct

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.common import generate_config
from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileOpen
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.stars_constant import StarsConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.os_manager import check_file_readable
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.utils import Utils
from framework.offset_calculator import FileCalculator
from framework.offset_calculator import OffsetCalculator
from mscalculate.aic.aic_utils import AicPmuUtils
from mscalculate.aic.pmu_calculator import PmuCalculator
from mscalculate.calculate_ai_core_data import CalculateAiCoreData
from msmodel.aic.aic_pmu_model import AicPmuModel
from msmodel.iter_rec.iter_rec_model import HwtsIterModel
from msmodel.stars.ffts_pmu_model import FftsPmuModel
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.stars.ffts_block_pmu import FftsBlockPmuBean
from profiling_bean.stars.ffts_pmu import FftsPmuBean


class FftsPmuCalculate(PmuCalculator, MsMultiProcess):
    """
    class used to parse ffts pmu data
    """
    FFTS_PMU_SIZE = 128
    PMU_LENGTH = 8
    STREAM_TASK_CONTEXT_KEY_FMT = "{0}-{1}-{2}"

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._result_dir = self.sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._iter_model = HwtsIterModel(self._result_dir)
        self._iter_range = self.sample_config.get(StrConstant.PARAM_ITER_ID)
        self._model = FftsPmuModel(self._result_dir, DBNameConstant.DB_RUNTIME, [])
        self._data_list = {}
        self._sample_json = ConfigMgr.read_sample_config(self._result_dir)
        self._file_list = file_list.get(DataTag.FFTS_PMU, [])
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))
        self._wrong_func_type_count = 0
        self._aic_pmu_events = AicPmuUtils.get_pmu_events(self._sample_json.get(StrConstant.AI_CORE_PMU_EVENTS))
        self._aiv_pmu_events = AicPmuUtils.get_pmu_events(
            self._sample_json.get(StrConstant.AI_VECTOR_CORE_PMU_EVENTS))
        self._is_mix_needed = ChipManager().is_chip_v4()
        self.block_dict = {}
        self.mix_pmu_dict = {}

    @staticmethod
    def _get_total_cycle_and_pmu_data(data: any, is_true: bool) -> tuple:
        """
        default value for pmu cycle list can be set to zero.
        """
        return (data.total_cycle, data.pmu_list) if is_true else (0, [0] * FftsPmuCalculate.PMU_LENGTH)

    @staticmethod
    def _is_not_mix_main_core(core_data, data_type) -> bool:
        return bool((core_data.is_ffts_mix_aic_data() and data_type == 'aiv') or
                    (core_data.is_ffts_mix_aiv_data() and data_type == 'aic'))

    @staticmethod
    def __get_mix_type(data: any) -> str:
        """
        get mix type
        """
        if data.ffts_type != 4:
            return ''
        if data.subtask_type == 6:
            return Constant.TASK_TYPE_MIX_AIC
        if data.subtask_type == 7:
            return Constant.TASK_TYPE_MIX_AIV
        return ''

    def ms_run(self: any) -> None:
        config = generate_config(PathManager.get_sample_json_path(self._result_dir))
        if config.get(StrConstant.AICORE_PROFILING_MODE) == StrConstant.AIC_SAMPLE_BASED_MODE \
                or config.get(StrConstant.AIV_PROFILING_MODE) == StrConstant.AIC_SAMPLE_BASED_MODE:
            return
        self.init_params()
        self.calculate()
        self.save()

    def calculate(self: any) -> None:
        if ProfilingScene().is_operator():
            self._parse_all_file()
        else:
            self._parse_by_iter()

    def save(self: any) -> None:
        """
        save parser data to db
        :return: None
        """
        if not self._data_list.get(StrConstant.CONTEXT_PMU_TYPE):
            logging.warning("No ai core pmu data found, data list is empty!")
            return
        if self._wrong_func_type_count:
            logging.warning("Some PMU data fails to be parsed, err count: %s", self._wrong_func_type_count)

        pmu_data = []
        for data in self._data_list.get(StrConstant.BLOCK_PMU_TYPE, []):
            self.calculate_block_pmu_list(data)
        self.add_block_pmu_list()
        if self._is_mix_needed:
            self.calculate_mix_pmu_list(pmu_data)
        else:
            self.calculate_pmu_list(pmu_data)
        pmu_data.sort(key=lambda x: x[-3])
        try:
            with self._model as _model:
                _model.flush(pmu_data)
        except sqlite3.Error as err:
            logging.error("Save ffts pmu data failed! %s", err)

    def calculate_mix_pmu_list(self: any, pmu_data_list: list) -> None:
        """
        calculate pmu
        :param pmu_data_list: out args
        :return:
        """
        for data in self._data_list.get(StrConstant.CONTEXT_PMU_TYPE, []):
            task_type = 0 if data.is_aic_data() else 1

            aic_pmu_value, aiv_pmu_value, aic_total_cycle, aiv_total_cycle = self.get_total_cycle_info(data, task_type)

            aic_total_time = self.calculate_total_time(aic_total_cycle, data)
            aiv_total_time = self.calculate_total_time(aiv_total_cycle, data, data_type='aiv')
            aic_calculator = CalculateAiCoreData(self._project_path)
            aic_pmu_value = aic_calculator.add_pipe_time(
                aic_pmu_value, aic_total_time, self._sample_json.get('ai_core_metrics'))
            aiv_pmu_value = aic_calculator.add_pipe_time(
                aiv_pmu_value, aiv_total_time, self._sample_json.get('ai_core_metrics'))

            aic_pmu_value_list = list(
                itertools.chain.from_iterable(PmuMetrics(aic_pmu_value).get_pmu_by_event_name(aic_pmu_value)))
            aiv_pmu_value_list = list(
                itertools.chain.from_iterable(PmuMetrics(aiv_pmu_value).get_pmu_by_event_name(aiv_pmu_value)))
            pmu_data = [
                aic_total_time, aic_total_cycle, *aic_pmu_value_list,
                aiv_total_time, aiv_total_cycle, *aiv_pmu_value_list,
                data.task_id, data.stream_id, data.subtask_id, data.subtask_type,
                InfoConfReader().time_from_syscnt(data.time_list[0]),
                InfoConfReader().time_from_syscnt(data.time_list[1]), data.ffts_type, task_type
            ]
            pmu_data_list.append(pmu_data)

    def calculate_pmu_list(self: any, pmu_data_list: list) -> None:
        """
        calculate pmu
        :param pmu_data_list: pmu events list
        :return:
        """
        self.__update_model_instance()
        for data in self._data_list.get(StrConstant.CONTEXT_PMU_TYPE, []):
            task_type = 0 if data.is_aic_data() else 1
            pmu_list = {}
            pmu_events = AicPmuUtils.get_pmu_events(self._sample_json.get('ai_core_profiling_events'))
            total_time = self.calculate_total_time(data.total_cycle, data)
            aic_calculator = CalculateAiCoreData(self._project_path)
            _, pmu_list = aic_calculator.compute_ai_core_data(
                Utils.generator_to_list(pmu_events), pmu_list, data.total_cycle, data.pmu_list)
            pmu_list = aic_calculator.add_pipe_time(pmu_list, total_time, self._sample_json.get('ai_core_metrics'))
            AicPmuUtils.remove_redundant(pmu_list)
            pmu_data = [
                total_time, data.total_cycle, *list(itertools.chain.from_iterable(pmu_list.values())), data.task_id,
                data.stream_id, task_type
            ]
            pmu_data_list.append(pmu_data)

    def calculate_total_time(self: any, total_cycle: int, data: any, data_type: str = 'aic') -> float:
        total_time = 0
        core_num = self._core_num_dict.get(data_type)
        block_dim = self._get_current_block('block_dim', data)
        mix_block_dim = self._get_current_block('mix_block_dim', data)
        if self._is_not_mix_main_core(data, data_type):
            block_dim = mix_block_dim
        if all([block_dim, core_num, int(self._freq)]):
            total_time = total_cycle * 1000 * NumberConstant.NS_TO_US / int(self._freq) / block_dim * \
                         ((block_dim + core_num - 1) // core_num)
        return round(total_time, NumberConstant.ROUND_TWO_DECIMAL)

    def get_total_cycle_info(self, data: any, task_type: int) -> tuple:
        """
        add block mix pmu to context pmu
        :return: tuple
        """
        aic_pmu_value = self._get_pmu_value(*self._get_total_cycle_and_pmu_data(data, data.is_aic_data()),
                                            self._aic_pmu_events)
        aiv_pmu_value = self._get_pmu_value(*self._get_total_cycle_and_pmu_data(data, not data.is_aic_data()),
                                            self._aiv_pmu_events)
        data_key = self.STREAM_TASK_CONTEXT_KEY_FMT.format(data.stream_id, data.task_id, data.subtask_id)
        mix_pmu_info = self.mix_pmu_dict.get(data_key, {})
        aic_total_cycle = data.total_cycle if not task_type else 0
        mix_total_cycle = data.total_cycle if task_type else 0
        if mix_pmu_info:
            mix_total_cycle = mix_pmu_info.get('total_cycle')
            mix_data_type = mix_pmu_info.get('mix_type')
            if mix_data_type == 'mix_aiv':
                aic_pmu_value = mix_pmu_info.get('pmu')
                aic_total_cycle, mix_total_cycle = mix_total_cycle, aic_total_cycle
            else:
                aiv_pmu_value = mix_pmu_info.get('pmu')
        res_tuple = aic_pmu_value, aiv_pmu_value, aic_total_cycle, mix_total_cycle
        return res_tuple

    def calculate_block_pmu_list(self: any, data: any) -> None:
        """
        assortment mix pmu data
        :return: None
        """
        mix_type = self.__get_mix_type(data)
        if not mix_type:
            return
        task_key = self.STREAM_TASK_CONTEXT_KEY_FMT.format(data.stream_id, data.task_id, data.subtask_id)
        aic_pmu_value = self._get_total_cycle_and_pmu_data(data, data.is_aic_data())
        aiv_pmu_value = self._get_total_cycle_and_pmu_data(data, not data.is_aic_data())

        pmu_list = aic_pmu_value if mix_type == Constant.TASK_TYPE_MIX_AIC else aiv_pmu_value
        self.block_dict.setdefault(task_key, []).append((mix_type, pmu_list))

    def add_block_pmu_list(self) -> None:
        """
        add block log pmu which have same key: task_id-stream_id-subtask_id
        demo input: {'1-1-1': [['mix_aic', (100, [1,1,2,2,3,3,4,4])], ['mix_aic', (100, [4,4,3,3,2,2,1,1])]]}
            output: {'1-1-1': [['mix_aic', 200, [5,5,5,5,5,5,5,5]}
        """
        if not self.block_dict:
            return
        for key, value in self.block_dict.items():
            mix_type = {mix_type[0] for mix_type in value}
            # One mix operator has only one mix_type.
            if len(mix_type) != 1:
                logging.debug('Pmu data type error, task key: task_id-stream_id-subtask_id: %s', key)
                continue
            total_cycle = sum(cycle[1][0] for cycle in value)
            cycle_list = [sum(cycle[1][1][index] for cycle in value) for index in range(self.PMU_LENGTH)]
            mix_type_value = mix_type.pop()
            if mix_type_value == Constant.TASK_TYPE_MIX_AIC:
                pmu = self._get_pmu_value(total_cycle, cycle_list,
                                          self._aiv_pmu_events)
            elif mix_type_value == Constant.TASK_TYPE_MIX_AIV:
                pmu = self._get_pmu_value(total_cycle, cycle_list,
                                          self._aic_pmu_events)
            else:
                logging.debug('Mix type error, the key: task_id-stream_id-subtask_id: %s', key)
                pmu = cycle_list
            self.mix_pmu_dict[key] = {
                'mix_type': mix_type_value,
                'total_cycle': total_cycle,
                'pmu': pmu
            }

    def _parse_all_file(self) -> None:
        if not self._need_to_analyse():
            return
        try:
            for _file in self._file_list:
                file_path = PathManager.get_data_file_path(self._result_dir, _file)
                self._parse_binary_file(file_path)
        except (OSError, SystemError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _parse_by_iter(self) -> None:
        runtime_db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_RUNTIME)
        if os.path.exists(runtime_db_path):
            with self._model as model:
                model.drop_table(DBNameConstant.TABLE_METRICS_SUMMARY)
        with self._iter_model as iter_model:
            if not iter_model.check_db() or not iter_model.check_table():
                return
            offset_count, total_count = self._get_offset_and_total(self._iter_range)
            if total_count <= 0:
                logging.warning("The ffts pmu data that is not satisfied by the specified iteration!")
                return
            _file_calculator = FileCalculator(self._file_list, self.FFTS_PMU_SIZE, self._result_dir,
                                              offset_count, total_count)
            for chunk in Utils.chunks(_file_calculator.prepare_process(), self.FFTS_PMU_SIZE):
                self._get_pmu_decode_data(chunk)

    def _get_total_aic_count(self: any) -> int:
        sum_file_size = 0
        for file in self._file_list:
            sum_file_size += os.path.getsize(PathManager.get_data_file_path(self._result_dir, file))
        return sum_file_size // self.FFTS_PMU_SIZE

    def _get_offset_and_total(self: any, iteration: IterationRange) -> (int, int):
        """
        :param iteration:
        :return: offset count and total aic count
        """
        offset_count, total_count = self._iter_model.get_task_offset_and_sum(iteration, HwtsIterModel.AI_CORE_TYPE)
        _total_aic_count = self._get_total_aic_count()
        _sql_aic_count = self._iter_model.get_aic_sum_count()
        # get offset by all aic count and sql record count
        offset_count = _total_aic_count - _sql_aic_count + offset_count
        if offset_count < 0:
            total_count += offset_count
            offset_count = 0
        return offset_count, total_count

    def _parse_binary_file(self: any, file_path: str) -> None:
        """
        read binary data an decode
        :param file_path:
        :return:
        """
        check_file_readable(file_path)
        offset_calculator = OffsetCalculator(self._file_list, self.FFTS_PMU_SIZE, self._result_dir)
        with FileOpen(file_path, 'rb') as _pmu_file:
            _file_size = os.path.getsize(file_path)
            file_data = offset_calculator.pre_process(_pmu_file.file_reader, _file_size)
            for chunk in Utils.chunks(file_data, self.FFTS_PMU_SIZE):
                self._get_pmu_decode_data(chunk)

    def _get_pmu_decode_data(self: any, bin_data: bytes) -> any:
        try:
            func_type, _ = struct.unpack(StructFmt.STARS_HEADER_FMT, bin_data[:4])
        except (IndexError, ValueError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR) from err
        if Utils.get_func_type(func_type) == StarsConstant.FFTS_PMU_TAG:
            self._data_list.setdefault(StrConstant.CONTEXT_PMU_TYPE, []).append(FftsPmuBean.decode(bin_data))
        elif Utils.get_func_type(func_type) == StarsConstant.FFTS_BLOCK_PMU_TAG:
            self._data_list.setdefault(StrConstant.BLOCK_PMU_TYPE, []).append(FftsBlockPmuBean.decode(bin_data))
        else:
            self._wrong_func_type_count += 1
            logging.debug('Func type error, data may have been lost. Func type: %s', func_type)

    def _need_to_analyse(self: any) -> bool:
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_RUNTIME)
        if not os.path.exists(db_path):
            return True
        if DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_METRICS_SUMMARY):
            return False
        return True

    def _get_pmu_value(self, total_cycle, pmu_list, pmu_metrics) -> list:
        _, pmu_list = CalculateAiCoreData(self._result_dir).compute_ai_core_data(pmu_metrics, {}, total_cycle, pmu_list)
        return pmu_list

    def _get_current_block(self: any, block_type: str, ai_core_data: any) -> any:
        """
        get the current block dim when stream id and task id occurs again
        :param ai_core_data: ai core pmu data
        :return: block dim
        """
        block_key = self.STREAM_TASK_CONTEXT_KEY_FMT.format(ai_core_data.stream_id,
                                                            ai_core_data.task_id, ai_core_data.subtask_id)
        if not ai_core_data.is_ffts_plus_type():
            block_key = self.STREAM_TASK_CONTEXT_KEY_FMT.format(ai_core_data.stream_id,
                                                                ai_core_data.task_id,
                                                                NumberConstant.DEFAULT_GE_CONTEXT_ID)
        block = self._block_dims.get(block_type, {}).get(block_key)
        if not block:
            return 0
        return block.pop(0) if len(block) > 1 else block[0]

    def _format_ge_data(self: any, ge_data: list) -> None:
        for data in ge_data:
            if data.task_type not in [Constant.TASK_TYPE_AI_CORE, Constant.TASK_TYPE_AIV,
                                      Constant.TASK_TYPE_MIX_AIC, Constant.TASK_TYPE_MIX_AIV]:
                continue
            _key = self.STREAM_TASK_CONTEXT_KEY_FMT.format(data.stream_id, data.task_id, data.context_id)
            self._block_dims['block_dim'].setdefault(_key, []).append(int(data.block_dim))
            if data.task_type in [Constant.TASK_TYPE_MIX_AIV, Constant.TASK_TYPE_MIX_AIC]:
                self._block_dims['mix_block_dim'].setdefault(_key, []).append(int(data.mix_block_dim))

    def __update_model_instance(self):
        self._model = AicPmuModel(self._project_path)


class PmuMetrics:
    """
    Pmu metrics for ai core and ai vector core.
    """

    def __init__(self, pmu_dict: dict):
        self.init(pmu_dict)

    def init(self, pmu_dict: dict):
        """
        construct ffts pmu data.
        """
        for pmu_name, pmu_value in pmu_dict.items():
            setattr(self, pmu_name, pmu_value)

    def get_pmu_by_event_name(self, event_name_list):
        """
        get pmu value list order by pmu event name list.
        """
        AicPmuUtils.remove_redundant(event_name_list)
        return [getattr(self, event_name, 0) for event_name in event_name_list if hasattr(self, event_name)]
