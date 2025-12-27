# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------
import logging
from collections import deque
from typing import Tuple

from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from mscalculate.ascend_task.host_task_collector import HostTaskCollector
from msmodel.sqe_type_map import SqeType
from msparser.interface.istars_parser import IStarsParser
from profiling_bean.stars.block_log_bean import BlockLogBean


class LogBaseParser(IStarsParser):
    """
    class used to parser task and block log
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__()
        self._result_dir = result_dir
        self._data_list = []
        self._mismatch_task = []
        self._start_functype = None
        self._end_functype = None

    def preprocess_data(self: any) -> None:
        """
        preprocess data list
        :return: NA
        """
        self._set_stream_id_by_host()
        self._data_list, self._mismatch_task = self.get_task_time()

    def get_task_time(self: any) -> Tuple[list, list]:
        """
        Categorize data_list into start log and end log, and calculate the task time
        :return: result data list
        """
        task_map = {}
        self._data_list.sort(key=lambda x: x.sys_cnt)
        # task id stream id func type
        for data in self._data_list:
            task_key = "{0},{1}".format(str(data.task_id), str(data.stream_id))
            task_map.setdefault(task_key, {}).setdefault(data.func_type, deque([])).append(data)

        matched_result = []
        remaining_data = []
        mismatch_count = 0
        for data_key, data_dict in task_map.items():
            start_que = data_dict.get(self._start_functype, [])
            end_que = data_dict.get(self._end_functype, [])
            while start_que and end_que:
                start_task = start_que[0]
                end_task = end_que[0]
                if start_task.sys_cnt > end_task.sys_cnt:
                    mismatch_count += 1
                    _ = end_que.popleft()
                    continue
                start_task = start_que.popleft()
                end_task = end_que.popleft()
                if isinstance(start_task, BlockLogBean):
                    block_time = InfoConfReader().time_from_syscnt(end_task.sys_cnt) - \
                                 InfoConfReader().time_from_syscnt(start_task.sys_cnt)
                    res = [
                        start_task.stream_id, start_task.task_id, start_task.block_id, \
                        SqeType().instance(start_task.task_type).name,
                        # start timestamp end timestamp duration
                        start_task.sys_cnt, end_task.sys_cnt, block_time,
                        start_task.core_type, start_task.core_id
                    ]

                else:
                    res = [
                        start_task.stream_id, start_task.task_id, start_task.acc_id, \
                        SqeType().instance(start_task.task_type).name,
                        # start timestamp end timestamp duration
                        start_task.sys_cnt, end_task.sys_cnt, end_task.sys_cnt - start_task.sys_cnt
                    ]
                matched_result.append(res)
            if len(start_que) > 1 or end_que:
                logging.debug("Task or block mismatch happen in %s, start_que size: %d, end_que size: %d",
                              data_key, len(start_que), len(end_que))
                mismatch_count += len(start_que)
                mismatch_count += len(end_que)
                # when mismatched, discards illegal task
                continue
            while start_que:
                start_task = start_que.popleft()
                remaining_data.append(start_task)
        if mismatch_count > 0:
            logging.error("There are %d tasks or block mismatching.", mismatch_count)

        return sorted(matched_result, key=lambda data: data[4]), remaining_data

    def flush(self: any) -> None:
        """
        flush all buffer data to db
        :return: NA
        """
        if not self._data_list:
            return
        if self._model.init():
            self.preprocess_data()
            self._model.flush(self._data_list)
            self._model.finalize()
            self._data_list.clear()
            self._data_list.extend(self._mismatch_task)
            self._mismatch_task.clear()

    def _set_stream_id_by_host(self):
        if not ChipManager().is_chip_v6():
            return
        device_id = InfoConfReader().get_device_id()
        host_task_dict = HostTaskCollector(self._result_dir).get_host_task_stream_table(int(device_id))
        for data in self._data_list:
            if data.task_id not in host_task_dict:
                logging.warning(f"Task ID {data.task_id} not found in host task")
                continue
            data.stream_id = host_task_dict.get(data.task_id)
