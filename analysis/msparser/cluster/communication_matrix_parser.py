#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import logging
from enum import IntEnum

from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import CommunicationMatrixInfo
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from msparser.cluster.meta_parser import HcclAnalysisTool
from msparser.cluster.meta_parser import MetaParser


class MatrixDataType(IntEnum):
    TRANSPORT_TYPE = 0
    TRANS_SIZE = 1
    TRANS_TIME = 2
    LARGE_PACKET_NUM = 3
    PACKET_NUM = 4


class CommunicationMatrixParser(MetaParser):
    """
    communication matrix data parser
    """
    def __init__(self: any, events_data) -> None:
        self.op_events_dict = events_data
        self.op_info = []

    @staticmethod
    def combine_link_info(total_list: list, part_list: list) -> None:
        """
        combine op's some link info
        """
        for value in MatrixDataType.__members__.values():
            if value == MatrixDataType.TRANSPORT_TYPE:
                total_list[value] = part_list[value]
            else:
                total_list[value] += part_list[value]

    @staticmethod
    def convert_link_info(link_key: str, link_value: list) -> dict:
        new_link_dict = dict()
        local_rank, remote_rank = link_key.split("-")
        new_link_dict['Src Rank'] = local_rank
        new_link_dict[CommunicationMatrixInfo.SRC_RANK] = local_rank
        new_link_dict[CommunicationMatrixInfo.DST_RANK] = remote_rank
        new_link_dict[CommunicationMatrixInfo.TRANSPORT_TYPE] = link_value[MatrixDataType.TRANSPORT_TYPE]
        new_link_dict[CommunicationMatrixInfo.TRANSIT_SIZE_MB] = link_value[MatrixDataType.TRANS_SIZE]
        new_link_dict[CommunicationMatrixInfo.TRANSIT_TIME_MS] = link_value[MatrixDataType.TRANS_TIME]
        new_link_dict[CommunicationMatrixInfo.BANDWIDTH_GB_S] = \
            HcclAnalysisTool.divide(link_value[MatrixDataType.TRANS_SIZE], link_value[MatrixDataType.TRANS_TIME])
        standard_bandwidth = HcclAnalysisTool.get_standard_bandwidth(). \
            get(HcclAnalysisTool.convert_to_str(link_value[MatrixDataType.TRANSPORT_TYPE]), -1)
        new_link_dict[CommunicationMatrixInfo.BANDWIDTH_UTILIZATION] = float(
            format(new_link_dict[CommunicationMatrixInfo.BANDWIDTH_GB_S] / standard_bandwidth, ".4f"))
        new_link_dict[CommunicationMatrixInfo.LARGE_PACKET_RATIO] = \
            HcclAnalysisTool.divide(link_value[MatrixDataType.LARGE_PACKET_NUM], link_value[MatrixDataType.PACKET_NUM])
        return new_link_dict

    def run(self: any) -> list:
        self.parse()
        self.combine()
        self.convert()
        return self.op_info

    def parse(self):
        for hccl_name, hccl_events in self.op_events_dict.items():
            self.parse_ops(hccl_events, hccl_name)
        if not self.op_info:
            logging.error("Fail to get op_info in Communication Matrix Parser")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

    def parse_ops(self: any, events: list, hccl_name: str):
        if not events:
            logging.error("Fail to get events data in Communication Matrix Parser")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
        link_info = {}
        idx = 0
        rdma_transit_op_num = NumberConstant.RDMA_NO_BARRIER_TASK_NUM
        if not HcclAnalysisTool.is_send_or_recv_op(events, idx):
            rdma_transit_op_num = NumberConstant.RDMA_WITH_BARRIER_TASK_NUM
        while idx < len(events):
            event = events[idx]
            if not HcclAnalysisTool.is_valid_link(event):
                idx += 1
                continue
            remote_rank = event.local_rank if int(event.remote_rank) == int("0xffffffff", 16) else event.remote_rank
            link_key = "{}-{}".format(event.local_rank, remote_rank)
            if event.transport_type == StrConstant.SDMA and event.hccl_name in StrConstant.SDMA_TRANSIT_ITEMS:
                if link_key not in link_info:
                    link_info[link_key] = [0] * len(MatrixDataType.__members__)
                trans_type = HcclAnalysisTool.get_transport_type(event)
                link_info[link_key][MatrixDataType.TRANSPORT_TYPE] = HcclAnalysisTool.convert_to_enum(trans_type)
                trans_size = HcclAnalysisTool.get_value(event.size, "size") / NumberConstant.COMMUNICATION_B_to_MB
                link_info[link_key][MatrixDataType.TRANS_SIZE] += trans_size
                link_info[link_key][MatrixDataType.TRANS_TIME] += \
                    HcclAnalysisTool.get_value(event.duration, "duration") / NumberConstant.NS_TO_MS
                link_info[link_key][MatrixDataType.PACKET_NUM] += 1
                if trans_size > HcclAnalysisTool.MessageSizeThreshold.get(trans_type, 0):
                    link_info[link_key][MatrixDataType.LARGE_PACKET_NUM] += 1
            if event.rdma_type == 'RDMA_SEND_PAYLOAD':
                saved_size, first_payload_time = 0, events[idx].timestamp
                while events[idx + 1].rdma_type == 'RDMA_SEND_PAYLOAD':
                    saved_size += events[idx].size
                    idx += 1
                time_elapsed = events[idx].timestamp - first_payload_time
                if link_key not in link_info:
                    link_info[link_key] = [0] * len(MatrixDataType.__members__)
                link_info[link_key][MatrixDataType.TRANSPORT_TYPE] =\
                    HcclAnalysisTool.convert_to_enum(event.transport_type)
                rdma_transit_result = HcclAnalysisTool.get_rdma_time_info(events, idx, rdma_transit_op_num)
                link_info[link_key][MatrixDataType.TRANS_TIME] += (rdma_transit_result[0] + time_elapsed /
                                                                   NumberConstant.NS_TO_MS)
                link_info[link_key][MatrixDataType.TRANS_SIZE] += (rdma_transit_result[1] + saved_size /
                                                                   NumberConstant.COMMUNICATION_B_to_MB)
                link_info[link_key][MatrixDataType.PACKET_NUM] += 1
                if rdma_transit_result[1] > HcclAnalysisTool.MessageSizeThreshold.get(event.transport_type, 0):
                    link_info[link_key][MatrixDataType.LARGE_PACKET_NUM] += 1
                idx += rdma_transit_op_num
                continue
            idx += 1
        hccl_dict = {StrConstant.OP_NAME: hccl_name, StrConstant.LINK_INFO: link_info}
        self.op_info.append(hccl_dict)

    def combine(self):
        """
        conclude all hccl ops to 'total ops'
        """
        total_dict = {}
        for hccl_dict in self.op_info:
            link_info = hccl_dict.get(StrConstant.LINK_INFO, {})
            for link_key, link_value in link_info.items():
                if link_key not in total_dict:
                    total_dict[link_key] = [0] * len(MatrixDataType.__members__)
                self.combine_link_info(total_dict[link_key], link_value)
        self.op_info.append({StrConstant.OP_NAME: StrConstant.TOTAL, StrConstant.LINK_INFO: total_dict})

    def convert(self):
        """
        convert op_info to json's format
        """
        for hccl_dict in self.op_info:
            link_info = hccl_dict.get(StrConstant.LINK_INFO, {})
            link_info_list = []
            for link_key, link_value in link_info.items():
                link_info_list.append(self.convert_link_info(link_key, link_value))
            hccl_dict[StrConstant.LINK_INFO] = link_info_list
