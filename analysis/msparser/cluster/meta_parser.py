#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import logging
from abc import abstractmethod
from collections import defaultdict
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_constant.str_constant import OpAnalysisType
from common_func.ms_constant.str_constant import OpBandWidthType
from common_func.ms_constant.str_constant import TransportType
from common_func.constant import Constant
from profiling_bean.db_dto.hccl_dto import HcclDto


class MetaParser:
    """
    abstract class for cluster communication and optimization suggestion
    """
    @abstractmethod
    def run(self):
        self.parse()

    @abstractmethod
    def parse(self):
        return


class HcclAnalysisTool:
    """
    support hccl parse
    """
    StandardBandWidth = {
        StrConstant.RDMA: NumberConstant.RDMA_BANDWIDTH,
        StrConstant.HCCS: NumberConstant.HCCS_BANDWIDTH,
        StrConstant.PCIE: NumberConstant.PCIE_BANDWIDTH
    }
    MessageSizeThreshold = {
        StrConstant.RDMA: NumberConstant.RDMA_MESSAGE_SIZE_THRESHOLD,
        StrConstant.HCCS: NumberConstant.HCCS_MESSAGE_SIZE_THRESHOLD,
        StrConstant.PCIE: NumberConstant.PCIE_MESSAGE_SIZE_THRESHOLD
    }

    @classmethod
    def get_value(cls: any, value: any, value_msg: str) -> float:
        if isinstance(value, int) or isinstance(value, float):
            return value
        if isinstance(value, str):
            logging.warning('%s is a string, not a int or float, please check', value_msg)
        if value is None:
            logging.warning('%s is a None value, please check', value_msg)
        return 0

    @classmethod
    def determine_rdma(cls: any, events: list, idx: int) -> bool:
        if idx > len(events) - NumberConstant.RDMA_TRANSIT_OP_NUM:
            return False
        second_task_type = events[idx + 1].task_type
        third_task_type = events[idx + 2].task_type
        if second_task_type == StrConstant.RDMA_SEND and third_task_type == StrConstant.NOTIFY_WAIT:
            return True
        else:
            return False

    @classmethod
    def get_rdma_time_info(cls: any, events: list, idx: int) -> list:
        transit_size = HcclAnalysisTool.get_value(events[idx].size, 'size') / (1024 ** 2)
        transit_time = 0
        for event in events[idx: idx + NumberConstant.RDMA_TRANSIT_OP_NUM]:
            transit_time += HcclAnalysisTool.get_value(event.duration, 'duration') / 1000
        return [transit_time, transit_size]

    @classmethod
    def init_dict(cls: any, keys: list) -> dict:
        return {key: 0 for key in keys}

    @classmethod
    def init_bandwidth_dict(cls) -> dict:
        dic = dict()
        # get public variables from OpAnalysisType
        values = [value for key, value in OpBandWidthType.__dict__.items() if '__' not in key]
        for trans_type in StrConstant.TRANSIT_TYPE:
            dic[trans_type] = HcclAnalysisTool.init_dict(values)
            dic[trans_type][OpBandWidthType.SIZE_DISTRIBUTION] = defaultdict(float)
        return dic

    @classmethod
    def get_transport_type(cls: any, src_id: int, dst_id: int):
        if src_id == dst_id or \
                src_id // NumberConstant.RANK_NUM_PER_SERVER != dst_id // NumberConstant.RANK_NUM_PER_SERVER:
            return StrConstant.LOCAL
        if src_id // NumberConstant.RANK_NUM_PER_OS != dst_id // NumberConstant.RANK_NUM_PER_OS:
            return StrConstant.PCIE
        return StrConstant.HCCS

    @classmethod
    def update_time_ratio(cls: any, op_time_dict: dict) -> None:
        try:
            op_time_dict[OpAnalysisType.WAIT_TIME_RATIO] = \
                round(op_time_dict.get(OpAnalysisType.WAIT_TIME) /
                      (op_time_dict.get(OpAnalysisType.WAIT_TIME) + op_time_dict.get(OpAnalysisType.TRANSIT_TIME)), 4)
        except ZeroDivisionError as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        try:
            op_time_dict[OpAnalysisType.SYNCHRONIZATION_TIME_RATIO] = \
                round(op_time_dict.get(OpAnalysisType.SYNCHRONIZATION_TIME)
                      / (op_time_dict.get(OpAnalysisType.SYNCHRONIZATION_TIME) +
                         op_time_dict.get(OpAnalysisType.TRANSIT_TIME)), 4)
        except ZeroDivisionError as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    @classmethod
    def update_bandwidth_record(cls: any, bandwidth_dict: dict, trans_type: str, size: float, dur: float) -> None:
        bandwidth_dict[trans_type][OpBandWidthType.TRANSIT_SIZE_MB] += size
        bandwidth_dict[trans_type][OpBandWidthType.TRANSIT_TIME_MS] += dur
        bandwidth_dict[trans_type][OpBandWidthType.SIZE_DISTRIBUTION][size] += 1

    @classmethod
    def combine_sdma_info(cls: any, bandwidth_dict: dict) -> None:
        bandwidth_dict[StrConstant.SDMA][OpBandWidthType.TRANSIT_SIZE_MB] = \
            bandwidth_dict[StrConstant.HCCS][OpBandWidthType.TRANSIT_SIZE_MB] + \
            bandwidth_dict[StrConstant.PCIE][OpBandWidthType.TRANSIT_SIZE_MB]
        bandwidth_dict[StrConstant.SDMA][OpBandWidthType.TRANSIT_TIME_MS] = \
            bandwidth_dict[StrConstant.HCCS][OpBandWidthType.TRANSIT_TIME_MS] + \
            bandwidth_dict[StrConstant.PCIE][OpBandWidthType.TRANSIT_TIME_MS]
        if bandwidth_dict[StrConstant.SDMA][OpBandWidthType.TRANSIT_TIME_MS] != 0:
            bandwidth_dict[StrConstant.SDMA][OpBandWidthType.BANDWIDTH_GB_S] = float(
                format(bandwidth_dict[StrConstant.SDMA][OpBandWidthType.TRANSIT_SIZE_MB] /
                       bandwidth_dict[StrConstant.SDMA][OpBandWidthType.TRANSIT_TIME_MS], ".4f"))

    @classmethod
    def analyze_bandwidth_info(cls: any, bandwidth_dict: dict, transport_type: str) -> None:
        if bandwidth_dict[transport_type][OpBandWidthType.TRANSIT_TIME_MS] != 0:
            bandwidth_dict[transport_type][OpBandWidthType.BANDWIDTH_GB_S] = float(
                format(bandwidth_dict[transport_type][OpBandWidthType.TRANSIT_SIZE_MB] /
                       bandwidth_dict[transport_type][OpBandWidthType.TRANSIT_TIME_MS], ".4f"))
        bandwidth_dict[transport_type][OpBandWidthType.BANDWIDTH_UTILIZATION] = float(
                format(bandwidth_dict[transport_type][OpBandWidthType.BANDWIDTH_GB_S] /
                       cls.StandardBandWidth.get(transport_type, -1), ".4f"))
        packet_num = 0
        large_packet_num = 0
        for size, count in bandwidth_dict[transport_type][OpBandWidthType.SIZE_DISTRIBUTION].items():
            if size > cls.MessageSizeThreshold.get(transport_type, 0):
                large_packet_num += count
            packet_num += count
        if packet_num > 0:
            bandwidth_dict[transport_type][OpBandWidthType.LARGE_PACKET_RATIO] = \
                round(large_packet_num / packet_num, 4)

    @classmethod
    def is_valid_link(cls: any, event: HcclDto) -> bool:
        src_rank_valid = event.src_rank is not None and event.src_rank != int("0xffffffff", 16)
        dst_rank_valid = event.dst_rank is not None and event.dst_rank != int("0xffffffff", 16)
        if src_rank_valid and dst_rank_valid:
            return True
        else:
            return False

    @classmethod
    def divide(cls: any, dividend: float, divisor: float):
        try:
            quotient = round(dividend / divisor, 4)
        except ZeroDivisionError as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return 0
        return quotient

    @classmethod
    def convert_to_enum(cls: any, trans_type: str) -> int:
        if trans_type == StrConstant.HCCS:
            return TransportType.HCCS
        if trans_type == StrConstant.PCIE:
            return TransportType.PCIE
        if trans_type == StrConstant.RDMA:
            return TransportType.RDMA
        logging.warning('trans_type is not normal, which is', trans_type)
        return -1

    @classmethod
    def convert_to_str(cls: any, trans_data_type: int) -> str:
        if trans_data_type == TransportType.HCCS:
            return StrConstant.HCCS
        if trans_data_type == TransportType.PCIE:
            return StrConstant.PCIE
        if trans_data_type == TransportType.RDMA:
            return StrConstant.RDMA
        logging.warning('trans_data_type is not normal, which is', trans_data_type)
        return 'Unknown transport type'

