#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

import logging

from msmodel.sqe_type_map import SqeType
from msmodel.stars.acsq_task_model import AcsqTaskModel
from msparser.interface.istars_parser import IStarsParser
from common_func.ms_constant.stars_constant import StarsConstant
from profiling_bean.stars.acsq_task import AcsqTask


class AcsqTaskParser(IStarsParser):
    """
    class used to parser acsq task log
    """

    def __init__(self: any, result_dir: str, db: str, table_list: list) -> None:
        super().__init__()
        self._model = AcsqTaskModel(result_dir, db, table_list)
        self._decoder = AcsqTask
        self._data_list = []

    def preprocess_data(self: any) -> None:
        """
        preprocess data list
        :return: NA
        """
        self._data_list = self.get_task_time()

    def get_task_time(self: any) -> list:
        """
        Categorize data_list into start log and end log, and calculate the task time
        :return: result data list
        """
        task_map = {}
        # task id stream id func type
        for data in self._data_list:
            task_key = "{0},{1}".format(str(data.task_id), str(data.stream_id))
            task_map.setdefault(task_key, {}).setdefault(data.func_type, []).append(data)

        result_list = []
        warning_status = False
        for data_key, data_dict in task_map.items():
            start_que = data_dict.get(StarsConstant.ACSQ_START_FUNCTYPE, [])
            end_que = data_dict.get(StarsConstant.ACSQ_END_FUNCTYPE, [])
            if len(start_que) != len(end_que):
                logging.debug("start_que not eq end que, data key %s", data_key)
                warning_status = True
            while start_que and end_que:
                start_task = start_que.pop()
                end_task = end_que.pop()
                result_list.append(
                    [start_task.stream_id, start_task.task_id, start_task.acc_id, SqeType(start_task.task_type).name,
                     # start timestamp end timestamp duration
                     start_task.sys_cnt, end_task.sys_cnt, end_task.sys_cnt - start_task.sys_cnt])

        if warning_status:
            logging.warning("Some task data are missing, set the log level to debug "
                            "and run again to know which tasks are missing.")
        return sorted(result_list, key=lambda data: data[4])
