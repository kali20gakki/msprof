#!/usr/bin/python3
# coding=utf-8
"""
function: runtime_api bean data
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

from profiling_bean.struct_info.struct_decoder import StructDecoder


class RunTimeApiBean(StructDecoder):
    """
    Runtime_api bean data for the data parsing by runtime_api parser.
    """

    def __init__(self: any, *args: any) -> None:
        _api_data = args[0]
        self._thread = _api_data[2]
        self._time_info = _api_data[3:5]
        self._data_size = _api_data[5]
        self._api = bytes.decode(_api_data[6]).split('\0')[0]
        self._ret_code = _api_data[7]
        self._task_tag = _api_data[8:30]
        self._copy_mes = _api_data[30]

    @property
    def thread(self: any) -> int:
        """
        runtime_api thread
        :return:
        """
        return self._thread

    @property
    def time_info(self: any) -> tuple:
        """
        runtime_api time_info
        time_info[0]:entry_time
        time_info[1]:exiTime
        :return:
        """
        return self._time_info

    @property
    def api(self: any) -> str:
        """
        runtime_api api
        :return:
        """
        return self._api

    @property
    def ret_code(self: any) -> int:
        """
        runtime_api ret_code
        :return:
        """
        return self._ret_code

    @property
    def task_tag(self: any) -> tuple:
        """
        runtime_api task_tag
        task_tag[0]: StreamID
        task_tag[1]: taskNum
        task_tag[2:]: taskId
        :return:
        """
        stream_id = self._task_tag[0]
        task_num = self._task_tag[1]
        task_id = list(map(str, self._task_tag[2:2 + 2*task_num:2]))
        batch_id = list(map(str, self._task_tag[3:3 + 2*task_num:2]))
        task_tag = (stream_id, task_num, ','.join(task_id), ','.join(batch_id))
        return task_tag

    @property
    def copy_mes(self: any) -> str:
        """
        runtime_api copy_mes
        :return:
        """
        return self._copy_mes

    @property
    def data_size(self: any) -> str:
        """
        runtime_api data_size
        :return:
        """
        return self._data_size
