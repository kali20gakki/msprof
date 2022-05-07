"""
This script is used to load training_trace data from db
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

from mscalculate.interface.step_trace_tag_handler import StepTraceTagHandler
from common_func.step_trace_constant import StepTraceConstant


class AllReduceTagHandler(StepTraceTagHandler):
    """
    get all reduce data
    """
    def __init__(self: any) -> None:
        self.collect_data = []

    def receive_record(self: any, record: dict) -> None:
        """
        receive record of step trace
        :param record: contain model_id, tag_id, timestamp
        :return: void
        """
        self.process_record(record)

    def get_data(self: any) -> list:
        """
        return data of this handler
        :return: dict
        """
        return self.collect_data

    def process_record(self: any, record: dict) -> None:
        """
        get reduce start, reduce end from record
        :param record: contain model_id, tag_id, timestamp
        :return: void
        """
        if not record[StepTraceConstant.TAG_ID] % 2:
            self.collect_data.append(
                {StepTraceConstant.REDUCE_START: record[StepTraceConstant.TIME_STAMP],
                 StepTraceConstant.REDUCE_END: None})
        if record[StepTraceConstant.TAG_ID] % 2 and self.collect_data:
            self.collect_data[-1][StepTraceConstant.REDUCE_END] = record[StepTraceConstant.TIME_STAMP]


class TrainingTraceTagHandler(StepTraceTagHandler):
    """
    get training trace data
    """
    def __init__(self: any) -> None:
        self.collect_data = {}

    def receive_record(self: any, record: dict) -> None:
        """
        receive record of step trace
        :param record: contain model_id, tag_id, timestamp
        :return: void
        """
        self.process_record(record)

    def get_data(self: any) -> list:
        """
        get reduce start, reduce end from record
        :param record: contain model_id, tag_id, timestamp
        :return: void
        """
        return self.collect_data

    def process_record(self: any, record: dict) -> None:
        """
        get bp, fp from record
        :param record: contain model_id, tag_id, timestamp
        :return: void
        """
        user_set_flag = False
        if record[StepTraceConstant.TAG_ID] == StepTraceConstant.FP_TAG:
            user_set_flag = True
            self.collect_data[StepTraceConstant.FORWARD_PROPAGATION] = record.get(StepTraceConstant.TIME_STAMP)

        if record[StepTraceConstant.TAG_ID] == StepTraceConstant.GE_FP_TAG and not user_set_flag:
            self.collect_data[StepTraceConstant.FORWARD_PROPAGATION] = record.get(StepTraceConstant.TIME_STAMP)

        if record[StepTraceConstant.TAG_ID] == StepTraceConstant.BP_TAG:
            self.collect_data[StepTraceConstant.BACK_PROPAGATION] = record.get(StepTraceConstant.TIME_STAMP)
