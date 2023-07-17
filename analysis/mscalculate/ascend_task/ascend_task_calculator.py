#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import logging
from typing import List
from dataclasses import astuple

from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.ms_constant.str_constant import StrConstant
from analyzer.scene_base.profiling_scene import ProfilingScene
from mscalculate.ascend_task.ascend_task import TopDownTask
from mscalculate.ascend_task.ascend_task_generator import AscendTaskGenerator
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.path_manager import PathManager
from msmodel.task_time.ascend_task_model import AscendTaskModel
from profiling_bean.db_dto.step_trace_dto import IterationRange


class AscendTaskCalculator(MsMultiProcess):
    """
    calculate ascend task and store in db
    notice: in future, delete this file
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self.sample_config = sample_config
        self.project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self.model = AscendTaskModel(self.project_path, [DBNameConstant.TABLE_ASCEND_TASK])

    def ms_run(self: any) -> None:
        """
        process
        :return: None
        """
        if not self._judge_calculate_again():
            return
        ascend_tasks = self._collect_ascend_tasks()
        self._save(ascend_tasks)

    def _collect_ascend_tasks(self):
        iter_range: IterationRange = self.sample_config.get(StrConstant.PARAM_ITER_ID)
        if not ProfilingScene().is_operator():
            return AscendTaskGenerator(self.project_path).run(iter_range.model_id,
                                                              iter_range.iteration_start, iter_range.iteration_end)
        else:
            # in this scene, default iteration is 1
            return AscendTaskGenerator(self.project_path).run(NumberConstant.INVALID_MODEL_ID,
                                                              iter_range.iteration_start, iter_range.iteration_end)

    def _save(self, ascend_tasks: List[TopDownTask]):
        if not ascend_tasks:
            return
        tasks_data = [astuple(task) for task in ascend_tasks]
        self.model.init()
        self.model.drop_table(DBNameConstant.TABLE_ASCEND_TASK)
        self.model.create_table()
        self.model.insert_data_to_db(DBNameConstant.TABLE_ASCEND_TASK, tasks_data)
        self.model.finalize()

    def _judge_calculate_again(self):
        if not ProfilingScene().is_operator():
            logging.info("In graph scene, to generate table %s", DBNameConstant.TABLE_ASCEND_TASK)
            return True
        else:
            ascend_task_db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_ASCEND_TASK)
            if DBManager.check_tables_in_db(ascend_task_db_path, DBNameConstant.TABLE_ASCEND_TASK):
                logging.info("Found table %s in operator scene, no need to generate again",
                             DBNameConstant.TABLE_ASCEND_TASK)
                return False
            logging.info("No table %s found, to generate it", DBNameConstant.TABLE_ASCEND_TASK)
            return True
