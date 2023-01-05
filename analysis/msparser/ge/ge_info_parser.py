#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import logging
import sqlite3

from common_func.constant import Constant
from common_func.ms_constant.str_constant import StrConstant
from common_func.db_name_constant import DBNameConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from msmodel.ge.ge_info_model import GeModel
from msparser.ge.ge_session_parser import GeSessionParser
from msparser.ge.ge_step_parser import GeStepParser
from msparser.ge.ge_task_parser import GeTaskParser
from msparser.ge.ge_tensor_parser import GeTensorParser
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag


class GeInfoParser(IParser, MsMultiProcess):
    """
    ge info data parser
    """
    GE_TASK = {MsProfCommonConstant.MODEL_ID: 0, MsProfCommonConstant.INDEX_ID: 9}
    GE_TENSOR = {MsProfCommonConstant.MODEL_ID: 0, MsProfCommonConstant.INDEX_ID: 10}
    GE_STEP = {MsProfCommonConstant.MODEL_ID: 0, MsProfCommonConstant.INDEX_ID: 5, MsProfCommonConstant.TAG: 6}

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._parsing_obj = [GeStepParser, GeTaskParser, GeTensorParser, GeSessionParser]
        self._model = None
        self.ge_info_data = {}
        self._table_list = []

    @staticmethod
    def offset_index(dynamic_model_dict: dict, index_space: dict):
        """
        make dynamic shape model index start from 1
        """
        for model_id, task_info_list in dynamic_model_dict.items():
            if not task_info_list:
                continue
            min_index_task = min(task_info_list, key=lambda x: x[index_space.get(MsProfCommonConstant.INDEX_ID)])
            offset = min_index_task[index_space.get(MsProfCommonConstant.INDEX_ID)] - 1
            for task in task_info_list:
                task[index_space.get(MsProfCommonConstant.INDEX_ID)] -= offset

    def parse(self: any) -> None:
        """
        parse rts data
        """
        for parse_class in self._parsing_obj:
            ge_info_data = parse_class(self._file_list, self._sample_config).ms_run()
            self.ge_info_data.update(ge_info_data)
            self._table_list.extend(list(ge_info_data.keys()))
        dynamic_data_db_name = [(DBNameConstant.TABLE_GE_TASK, self.GE_TASK),
                                (DBNameConstant.TABLE_GE_TENSOR, self.GE_TENSOR)]
        for db_name, index_space in dynamic_data_db_name:
            self.dynamic_scene_process(db_name, index_space)

    def dynamic_scene_process(self: any, db_name: str, index_space: dict) -> None:
        """
        check whether exists dynamic shape model and process them
        """
        ge_data = self.ge_info_data.get(db_name, [])
        ge_step_data = self.ge_info_data.get(DBNameConstant.TABLE_GE_STEP, [])
        dynamic_model_dict = {}
        if ge_data:
            # check whether include dynamic shape model
            for datum in ge_data:
                if datum[index_space.get(MsProfCommonConstant.INDEX_ID)] > 0:
                    dynamic_model_dict.setdefault(
                        datum[index_space.get(MsProfCommonConstant.MODEL_ID)], []).append(datum)
        # exist dynamic shape model
        if dynamic_model_dict:
            if not ge_step_data:
                logging.warning(f"dynamic shape model lack of ge step info data. "
                                f"dynamic profiling scene will miss ai core data")
            else:
                if not self.is_complete_iter_exist(ge_step_data):
                    return
                self.remove_incomplete_index(dynamic_model_dict, ge_step_data, index_space)
                self.offset_index(dynamic_model_dict, index_space)
                self.ge_info_data[db_name] = [
                    task
                    for task_list in dynamic_model_dict.values()
                    for task in task_list
                ]

    def remove_incomplete_index(self: any, dynamic_model_dict: dict, ge_step_data: list, index_space: dict) -> None:
        """
        remove incomplete iteration ahead caused by dynamic profiling
        """
        ge_step_dict = {}
        for datum in ge_step_data:
            ge_step_dict.setdefault(datum[self.GE_STEP.get(MsProfCommonConstant.MODEL_ID)], []).append(datum)
        for model_id in dynamic_model_dict:
            if model_id not in ge_step_dict:
                logging.warning(f"dynamic shape model lack part of ge step info data. "
                                f"dynamic profiling scene will miss ai core data")
            remove_iter_list = []
            # judge whether the smallest index has the tag 0, which means an iter start
            min_index_datum = min(ge_step_dict.get(model_id),
                                  key=lambda x: (x[self.GE_STEP.get(MsProfCommonConstant.INDEX_ID)],
                                                 x[self.GE_STEP.get(MsProfCommonConstant.TAG)]))
            try:
                if int(min_index_datum[self.GE_STEP.get(MsProfCommonConstant.TAG)]) != 0:
                    remove_iter_list.append(min_index_datum[self.GE_STEP.get(MsProfCommonConstant.INDEX_ID)])
            except ValueError:
                logging.warning('ge step table has a wrong value type in column tag!')
            dynamic_model_dict[model_id] = [
                task
                for task in dynamic_model_dict.get(model_id)
                if task[index_space.get(MsProfCommonConstant.INDEX_ID)] not in set(remove_iter_list)
            ]

    def is_complete_iter_exist(self: any, ge_step_data: list) -> bool:
        """
        check whether ge step info have a complete iteration for any model
        """
        record_dict = {}
        for ge_step in ge_step_data:
            model_index_key = (ge_step[self.GE_STEP.get(MsProfCommonConstant.MODEL_ID)],
                               ge_step[self.GE_STEP.get(MsProfCommonConstant.INDEX_ID)])
            if model_index_key not in record_dict:
                record_dict[model_index_key] = 1
            else:
                return True
        return False

    def save(self: any) -> None:
        """
        save data
        """
        self._model = GeModel(self._project_path, self._table_list)
        self._model.init()
        self._model.flush_all(self.ge_info_data)
        self._model.finalize()

    def ms_run(self: any) -> None:
        """
        parse and save ge info data
        :return:
        """
        if not self._file_list.get(DataTag.GE_TASK, []):
            return
        try:
            self.parse()
        except (OSError, IOError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        try:
            self.save()
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
