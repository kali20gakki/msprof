#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import logging
from collections import defaultdict
from common_func.constant import Constant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_constant.str_constant import OpAnalysisType
from common_func.ms_constant.str_constant import OpBandWidthType
from common_func.msprof_exception import ProfException
from msparser.cluster.meta_parser import MetaParser


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
        if src_id == dst_id or src_id // 8 != dst_id // 8:
            return StrConstant.LOCAL
        if src_id // 4 != dst_id // 4:
            return StrConstant.PCIE
        return StrConstant.HCCS

    @classmethod
    def update_time_ratio(cls: any, op_time_dict: dict) -> None:
        try:
            op_time_dict[OpAnalysisType.WAIT_TIME_RATIO] = \
                float(format(op_time_dict.get(OpAnalysisType.WAIT_TIME) /
                             (op_time_dict.get(OpAnalysisType.WAIT_TIME) +
                              op_time_dict.get(OpAnalysisType.TRANSIT_TIME)), ".4f"))
        except ZeroDivisionError as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        try:
            op_time_dict[OpAnalysisType.SYNCHRONIZATION_TIME_RATIO] = \
                float(format(op_time_dict.get(OpAnalysisType.SYNCHRONIZATION_TIME) /
                             (op_time_dict.get(OpAnalysisType.SYNCHRONIZATION_TIME) +
                              op_time_dict.get(OpAnalysisType.TRANSIT_TIME)), ".4f"))
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
                float(format(large_packet_num / packet_num, ".4f"))


class CommunicationParser(MetaParser):
    """
    cluster communication data parser
    """
    def __init__(self: any, events_data) -> None:
        self.op_events_dict = events_data
        self.op_info = {}

    def run(self: any) -> dict:
        self.parse()
        self.combine()
        return self.op_info

    def parse(self):
        for hccl_name in self.op_events_dict:
            self.parse_ops(self.op_events_dict.get(hccl_name, {}), hccl_name)
        if not self.op_info:
            logging.error("Fail to get op_info in Communication Parser")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

    def parse_ops(self: any, op_events: dict, hccl_name: str) -> None:
        """
        time and link info parser for every hccl operators
        """
        self.op_info[hccl_name] = {}
        for rank_id in op_events:
            self.op_info[hccl_name][rank_id] = {}
            if not op_events.get(rank_id):
                logging.error("Fail to get no.%s rank events info, communication parser is interrupted", str(rank_id))
                raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
            events = op_events.get(rank_id)
            # only choose main stream for op time analysis parser
            main_events = [event for event in events if event.plane_id == NumberConstant.MAIN_STREAM_THREAD_ID]
            if main_events:
                self.op_info[hccl_name][rank_id][StrConstant.COMMUNICATION_TIME_INFO] = self.op_time_parser(main_events)
            else:
                logging.error("Fail to get no.%s rank main events info,"
                              " communication parser is interrupted", str(rank_id))
                raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
            # choose all stream for Bandwidth analysis parser
            if events:
                self.op_info[hccl_name][rank_id][StrConstant.COMMNUNICATION_BANDWIDTH_INFO] \
                    = self.op_bandwidth_parser(events)
            else:
                logging.error("Fail to get no.%s rank events info, communication parser is interrupted", str(rank_id))
                raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

    @staticmethod
    def op_time_parser(main_events: list) -> dict:
        """
        time info parser
        """
        # in case there exists keys that never use, init dict first
        values = [value for key, value in OpAnalysisType.__dict__.items() if '__' not in key]
        op_time_dict = HcclAnalysisTool.init_dict(values)
        wait_flag = True
        idx = 0
        while idx < len(main_events):
            event = main_events[idx]
            if event.transport_type == StrConstant.SDMA and event.task_type in StrConstant.SDMA_TRANSIT_ITEMS:
                wait_flag = False
                op_time_dict[OpAnalysisType.TRANSIT_TIME] += \
                    HcclAnalysisTool.get_value(event.duration, "duration") / 1000
            if event.transport_type == StrConstant.RDMA and HcclAnalysisTool.determine_rdma(main_events, idx):
                wait_flag = False
                rdma_transit_result = HcclAnalysisTool.get_rdma_time_info(main_events, idx)
                op_time_dict[OpAnalysisType.TRANSIT_TIME] += rdma_transit_result[0]
                idx += NumberConstant.RDMA_TRANSIT_OP_NUM
                continue
            if event.task_type == StrConstant.NOTIFY_WAIT:
                wait_time = HcclAnalysisTool.get_value(event.duration, "duration") / 1000
                if wait_flag:
                    op_time_dict[OpAnalysisType.SYNCHRONIZATION_TIME] += wait_time
                op_time_dict[OpAnalysisType.WAIT_TIME] += wait_time
            idx += 1
        op_time_dict[OpAnalysisType.ELAPSE_TIME] = \
            (main_events[-1].timestamp + main_events[-1].duration - main_events[0].timestamp) / 1000
        HcclAnalysisTool.update_time_ratio(op_time_dict)
        return op_time_dict

    @staticmethod
    def op_bandwidth_parser(events: list) -> dict:
        """
        Bandwidth info parser
        """
        op_bandwidth_dict = HcclAnalysisTool.init_bandwidth_dict()
        idx = 0
        while idx < len(events):
            event = events[idx]
            if event.transport_type == StrConstant.SDMA and event.task_type in StrConstant.SDMA_TRANSIT_ITEMS:
                transport_type = HcclAnalysisTool.get_transport_type(event.src_rank, event.dst_rank)
                # do not consider local copy
                if transport_type == StrConstant.LOCAL:
                    idx += 1
                    continue
                HcclAnalysisTool.update_bandwidth_record(op_bandwidth_dict, transport_type,
                                                         HcclAnalysisTool.get_value(event.size, "size") / (1024 ** 2),
                                                         HcclAnalysisTool.get_value(event.duration, "duration") / 1000)
            if event.transport_type == StrConstant.RDMA and HcclAnalysisTool.determine_rdma(events, idx):
                rdma_transit_result = HcclAnalysisTool.get_rdma_time_info(events, idx)
                HcclAnalysisTool.update_bandwidth_record(op_bandwidth_dict, event.transport_type,
                                                         rdma_transit_result[1],
                                                         rdma_transit_result[0])
                idx += NumberConstant.RDMA_TRANSIT_OP_NUM
                continue
            idx += 1
        for transport_type in StrConstant.TRANSIT_TYPE:
            if transport_type == StrConstant.SDMA:
                HcclAnalysisTool.combine_sdma_info(op_bandwidth_dict)
            else:
                HcclAnalysisTool.analyze_bandwidth_info(op_bandwidth_dict, transport_type)
        return op_bandwidth_dict

    def combine(self):
        """
        conclude all hccl ops to 'total ops'
        """
        self.op_info[StrConstant.TOTAL] = {}
        for hccl_name, hccl_dict in self.op_info.items():
            if hccl_name == StrConstant.TOTAL:
                continue
            for rank_id, rank_dict in hccl_dict.items():
                if rank_id not in self.op_info[StrConstant.TOTAL]:
                    self.op_info[StrConstant.TOTAL][rank_id] = {}
                self.combine_ops_info(rank_dict, self.op_info[StrConstant.TOTAL][rank_id])

    def combine_ops_info(self, rank_dict: dict, total_ops_dict: dict) -> None:
        for com_info, com_info_dict in rank_dict.items():
            if com_info == StrConstant.COMMUNICATION_TIME_INFO:
                if com_info not in total_ops_dict:
                    # get public variables from OpAnalysisType
                    values = [value for key, value in OpAnalysisType.__dict__.items() if '__' not in key]
                    total_ops_dict[com_info] = HcclAnalysisTool.init_dict(values)
                self.combine_ops_time_info(rank_dict[com_info], total_ops_dict[com_info])
            if com_info == StrConstant.COMMNUNICATION_BANDWIDTH_INFO:
                if com_info not in total_ops_dict:
                    total_ops_dict[com_info] = HcclAnalysisTool.init_bandwidth_dict()
                self.combine_ops_bandwidth_info(rank_dict[com_info], total_ops_dict[com_info])
        return

    @staticmethod
    def combine_ops_time_info(part_dict: dict, total_dict: dict) -> None:
        ratio_list = [OpAnalysisType.WAIT_TIME_RATIO, OpAnalysisType.SYNCHRONIZATION_TIME_RATIO]
        # first level combine
        for key, value in part_dict.items():
            if key not in ratio_list:
                total_dict[key] += value
        # second level combine
        HcclAnalysisTool.update_time_ratio(total_dict)
        return

    def combine_ops_bandwidth_info(self: any, part_dict: dict, total_dict: dict) -> None:
        add_list = [OpBandWidthType.TRANSIT_TIME_MS, OpBandWidthType.TRANSIT_SIZE_MB]
        dict_list = [OpBandWidthType.SIZE_DISTRIBUTION]
        # first level combine
        for transport_type, part_transport_dict in part_dict.items():
            if transport_type == StrConstant.SDMA:
                continue
            for bandwidth_msg, value in part_transport_dict.items():
                if bandwidth_msg in add_list:
                    total_dict[transport_type][bandwidth_msg] += value
                if bandwidth_msg in dict_list:
                    self.combine_size_distribution(value, total_dict[transport_type][bandwidth_msg])
        # second level combine
        for transport_type in StrConstant.TRANSIT_TYPE:
            if transport_type == StrConstant.SDMA:
                HcclAnalysisTool.combine_sdma_info(total_dict)
            else:
                HcclAnalysisTool.analyze_bandwidth_info(total_dict, transport_type)

    @staticmethod
    def combine_size_distribution(part_dist_dict: dict, total_dist_dict: dict):
        for size, cnt in part_dist_dict.items():
            total_dist_dict[size] += cnt
