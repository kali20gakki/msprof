#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import logging
from profiling_bean.db_dto.hccl_dto import HcclDto
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_constant.str_constant import OpAnalysisType
from common_func.ms_constant.str_constant import OpBandWidthType
from common_func.msprof_exception import ProfException
from msparser.cluster.meta_parser import MetaParser
from msparser.cluster.meta_parser import HcclAnalysisTool


class CommunicationParser(MetaParser):
    """
    cluster communication data parser
    """
    def __init__(self: any, events_data) -> None:
        self.op_events_dict = events_data
        self.op_info = {}

    @staticmethod
    def combine_size_distribution(part_dist_dict: dict, total_dist_dict: dict):
        for size, cnt in part_dist_dict.items():
            total_dist_dict[size] += cnt

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

    @staticmethod
    def is_transit_sdma_event(event: HcclDto) -> bool:
        if event.transport_type == StrConstant.SDMA and event.task_type in StrConstant.SDMA_TRANSIT_ITEMS and \
                HcclAnalysisTool.get_transport_type(event.src_rank, event.dst_rank) != StrConstant.LOCAL:
            return True
        else:
            return False

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
            if CommunicationParser.is_transit_sdma_event(event):
                wait_flag = False
                op_time_dict[OpAnalysisType.TRANSIT_TIME] += \
                    HcclAnalysisTool.get_value(event.duration, "duration") / NumberConstant.US_TO_MS
            if event.transport_type == StrConstant.RDMA and HcclAnalysisTool.determine_rdma(main_events, idx):
                wait_flag = False
                rdma_transit_result = HcclAnalysisTool.get_rdma_time_info(main_events, idx)
                op_time_dict[OpAnalysisType.TRANSIT_TIME] += rdma_transit_result[0]
                idx += NumberConstant.RDMA_TRANSIT_OP_NUM
                continue
            if event.task_type == StrConstant.NOTIFY_WAIT:
                wait_time = HcclAnalysisTool.get_value(event.duration, "duration") / NumberConstant.US_TO_MS
                if wait_flag:
                    op_time_dict[OpAnalysisType.SYNCHRONIZATION_TIME] += wait_time
                op_time_dict[OpAnalysisType.WAIT_TIME] += wait_time
            idx += 1
        op_time_dict[OpAnalysisType.ELAPSE_TIME] = \
            (main_events[-1].timestamp + main_events[-1].duration - main_events[0].timestamp) / NumberConstant.US_TO_MS
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
                HcclAnalysisTool.update_bandwidth_record(
                    op_bandwidth_dict, transport_type,
                    HcclAnalysisTool.get_value(event.size, "size") / NumberConstant.B_to_MB,
                    HcclAnalysisTool.get_value(event.duration, "duration") / NumberConstant.US_TO_MS)
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

    def run(self: any) -> dict:
        self.parse()
        self.combine()
        return self.op_info

    def parse(self):
        for hccl_name, op_events in self.op_events_dict.items():
            self.parse_ops(op_events, hccl_name)
        if not self.op_info:
            logging.error("Fail to get op_info in Communication Parser")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

    def parse_ops(self: any, op_events: dict, hccl_name: str) -> None:
        """
        time and link info parser for every hccl operators
        """
        self.op_info[hccl_name] = {}
        for rank_id in op_events:
            self.op_info.get(hccl_name).setdefault(rank_id, {})
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
                self.combine_ops_time_info(com_info_dict, total_ops_dict[com_info])
            if com_info == StrConstant.COMMNUNICATION_BANDWIDTH_INFO:
                if com_info not in total_ops_dict:
                    total_ops_dict[com_info] = HcclAnalysisTool.init_bandwidth_dict()
                self.combine_ops_bandwidth_info(com_info_dict, total_ops_dict[com_info])
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
