# coding=utf-8
"""
function: script used to parse acsq task data and save it to db
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
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
        # task_map is {'task_id, stream_id', {'0': list, '1': list }} and 0 is task start and 1 is end
        task_map = {}
        # task id stream id func type
        for data in self._data_list:
            task_key = "{0},{1}".format(str(data.task_id), str(data.stream_id))
            if task_key in task_map:
                data_list = task_map.get(task_key).setdefault(data.func_type, [])
                data_list.append(data)
            else:
                task_map[task_key] = {data.func_type: [data]}

        result_list = []
        for data_dict in task_map.values():
            start_que = data_dict.get(StarsConstant.ACSQ_START_FUNCTYPE, [])
            end_que = data_dict.get(StarsConstant.ACSQ_END_FUNCTYPE, [])
            if len(start_que) != len(end_que):
                logging.warning("start_que not eq end que")
            while start_que and end_que:
                start_task = start_que.pop()
                end_task = end_que.pop()
                result_list.append(
                    [start_task.stream_id, start_task.task_id, start_task.acc_id, SqeType(start_task.task_type).name,
                     # start timestamp end timestamp duration
                     start_task.sys_cnt, end_task.sys_cnt, end_task.sys_cnt - start_task.sys_cnt])

        return sorted(result_list, key=lambda data: data[4])
