#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import logging
from typing import List
from typing import Tuple

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from mscalculate.ascend_task.ascend_task import HostTask
from msmodel.runtime.runtime_host_task_model import RuntimeHostTaskModel


class HostTaskCollector:
    def __init__(self: any, result_dir: str):
        self.result_dir = result_dir

    @classmethod
    def _generate_host_task_objs(cls: any, raw_data: list):
        return [
            HostTask(*data) for data in raw_data
        ]

    @classmethod
    def _filter_dev_visible_host_tasks(cls: any, all_host_tasks: List[HostTask],
                                       dev_visible_ids: List[Tuple]) -> List[HostTask]:

        return list(filter(lambda x: (x.stream_id, x.task_id, x.batch_id) in dev_visible_ids, all_host_tasks))

    def get_host_tasks_by_model_and_iter(self: any, model_id: int, iter_id: int) -> List[HostTask]:
        """
        This function will get host tasks which are visible on the device side that with model within iter.
        Notice: Now, device tasks are all ge and hccl tasks, if new scene in future, expand it
        """
        if not self._check_host_tasks_exists():
            return []

        dev_visible_host_tasks = self._get_host_tasks(is_all=False, model_id=model_id,
                                                      iter_id=iter_id)

        if not dev_visible_host_tasks:
            logging.error("Get dev visible hosts for model_id: %d, iter_id: %d error.", model_id, iter_id)

        return dev_visible_host_tasks

    def get_host_tasks(self) -> List[HostTask]:
        """
        This function will get host tasks which are visible on the device side that with model within iter.
        Notice: Now, device tasks are all ge and hccl tasks, if new scene in future, expand it
        """
        if not self._check_host_tasks_exists():
            return []

        dev_visible_host_tasks = self._get_host_tasks(is_all=True, model_id=NumberConstant.INVALID_MODEL_ID,
                                                      iter_id=NumberConstant.INVALID_ITER_ID)

        if not dev_visible_host_tasks:
            logging.error("Get dev visible hosts error.")

        return dev_visible_host_tasks

    def _get_host_tasks(self: any, is_all: bool, model_id: int, iter_id: int) -> List[HostTask]:
        """
        get host tasks
        filter_: host tasks unique id range
        :return:
            is_all Ture: all host tasks
            is_all False: host tasks with model_id within iter
        """
        if not self._check_host_tasks_exists():
            return []
        with RuntimeHostTaskModel(self.result_dir) as model:
            host_tasks = model.get_host_tasks(is_all, model_id, iter_id)
            return self._generate_host_task_objs(host_tasks)

    def _check_host_tasks_exists(self):
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_RUNTIME)
        if not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_HOST_TASK):
            logging.warning("No table %s.%s found", DBNameConstant.DB_RUNTIME, DBNameConstant.TABLE_HOST_TASK)
            return False
        return True
