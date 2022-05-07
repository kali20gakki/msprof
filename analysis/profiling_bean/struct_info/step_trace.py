"""
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

from profiling_bean.struct_info.struct_decoder import StructDecoder


class StepTrace(StructDecoder):
    """
    step trace
    """

    def __init__(self: any, *args: any) -> None:
        step_trace = args[0]
        self._timestamp = step_trace[4]
        self._index_id = step_trace[5]
        self._model_id = step_trace[6]
        self._stream_id = step_trace[7]
        self._task_id = step_trace[8]
        self._tag_id = step_trace[9]

    @property
    def index_id(self: any) -> str:
        """
        get mode
        :return: mode
        """
        return self._index_id

    @property
    def model_id(self: any) -> int:
        """
        get task cyc
        :return: task cyc
        """
        return self._model_id

    @property
    def timestamp(self: any) -> int:
        """
        get timestamp
        :return: timestamp
        """
        return self._timestamp

    @property
    def stream_id(self: any) -> int:
        """
        get count_num
        :return: count_num
        """
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        """
        get core id
        :return: core_id
        """
        return self._task_id

    @property
    def tag_id(self: any) -> int:
        """
        get core id
        :return: core_id
        """
        return self._tag_id
