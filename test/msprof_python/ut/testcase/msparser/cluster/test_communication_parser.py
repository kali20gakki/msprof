#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import unittest
import os
from unittest import mock
import pytest
from msparser.cluster.communication_parser import CommunicationParser
from msparser.cluster.meta_parser import HcclAnalysisTool
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from common_func.platform.chip_manager import ChipManager
from profiling_bean.prof_enum.chip_model import ChipModel

NAMESPACE = 'msparser.cluster.communication_parser'


class Event:
    def __init__(self, hccl_name: str):
        self.hccl_name = hccl_name
        self.size = 1024 ** 2
        self.duration = 1000
        self.transport_type = StrConstant.RDMA
        self.timestamp = 0


class TestHcclAnalysisTool(unittest.TestCase):
    def test_get_value(self):
        ret = HcclAnalysisTool.get_value(1, 'none')
        self.assertEqual(ret, 1)

    def test_get_value_str(self):
        ret = HcclAnalysisTool.get_value('1', 'none')
        self.assertEqual(ret, 0)

    def test_get_value_none(self):
        ret = HcclAnalysisTool.get_value(None, 'none')
        self.assertEqual(ret, 0)

    def test_determine_rdma1(self):
        events = [Event(StrConstant.RDMA_SEND), Event(StrConstant.RDMA_SEND), Event(StrConstant.NOTIFY_WAIT),
                  Event(StrConstant.RDMA_SEND), Event(StrConstant.NOTIFY_WAIT)]
        ret = HcclAnalysisTool.determine_rdma(events, 0)
        self.assertEqual(ret, True)

    def test_determine_rdma2(self):
        events = [Event(StrConstant.RDMA_SEND)]
        ret = HcclAnalysisTool.determine_rdma(events, 0)
        self.assertEqual(ret, False)

    def test_get_rdma_time_info(self):
        events = [Event(StrConstant.RDMA_SEND), Event(StrConstant.RDMA_SEND), Event(StrConstant.NOTIFY_WAIT),
                  Event(StrConstant.RDMA_SEND), Event(StrConstant.NOTIFY_WAIT)]
        ret = HcclAnalysisTool.get_rdma_time_info(events, 0)
        self.assertEqual(ret, [5, 1])

    def test_get_transport_type_hccs(self):
        ret = HcclAnalysisTool.get_transport_type(1, 0)
        self.assertEqual(ret, StrConstant.HCCS)

    def test_get_transport_type_pcie(self):
        ret = HcclAnalysisTool.get_transport_type(5, 0)
        self.assertEqual(ret, StrConstant.PCIE)

    def test_get_transport_type_hccs_chip_v4(self):
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0
        ret = HcclAnalysisTool.get_transport_type(5, 0)
        self.assertEqual(ret, StrConstant.HCCS)
        ChipManager().chip_id = ChipModel.CHIP_V1_1_0

    def test_get_transport_type_local1(self):
        ret = HcclAnalysisTool.get_transport_type(0, 0)
        self.assertEqual(ret, StrConstant.LOCAL)

    def test_get_transport_type_local2(self):
        ret = HcclAnalysisTool.get_transport_type(488546486, 0)
        self.assertEqual(ret, StrConstant.LOCAL)


class TestCommunicationParser(unittest.TestCase):
    def test_run(self):
        with mock.patch(NAMESPACE + '.CommunicationParser.parse'), \
                mock.patch(NAMESPACE + '.CommunicationParser.combine'):
            CommunicationParser({}).run()

    def test_parse(self):
        with pytest.raises(ProfException) as err:
            CommunicationParser({}).parse()
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_parse_ops(self):
        with pytest.raises(ProfException) as err:
            CommunicationParser({}).parse_ops({1: {}}, 'name')
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_op_time_parser(self):
        CommunicationParser({}).op_time_parser([Event(StrConstant.RDMA_SEND)])

    def test_op_bandwidth_parser(self):
        CommunicationParser({}).op_bandwidth_parser([Event(StrConstant.RDMA_SEND)])

    def test_combine_time(self):
        com = CommunicationParser({})
        com.op_info = {
            "allReduce_1_1": {
                0: {
                    "Communication Time Info": {
                        "Elapse Time(ms)": 2,
                        "Transit Time(ms)": 1,
                        "Wait Time(ms)": 1,
                        "Synchronization Time(ms)": 1,
                        "Wait Time Ratio": 0.5,
                        "Synchronization Time Ratio": 0.5
                    }
                }
            },
            "allReduce_1_2": {
                0: {
                    "Communication Time Info": {
                        "Elapse Time(ms)": 2,
                        "Transit Time(ms)": 1,
                        "Wait Time(ms)": 1,
                        "Synchronization Time(ms)": 1,
                        "Wait Time Ratio": 0.5,
                        "Synchronization Time Ratio": 0.5
                    }
                }
            }
        }
        com.combine()
        ret = com.op_info
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Time Info"]["Elapse Time(ms)"], 4)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Time Info"]["Transit Time(ms)"], 2)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Time Info"]["Wait Time(ms)"], 2)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Time Info"]["Synchronization Time(ms)"], 2)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Time Info"]["Wait Time Ratio"], 0.5)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Time Info"]["Synchronization Time Ratio"], 0.5)

    def test_combine_bandwidth(self):
        com = CommunicationParser({})
        com.op_info = {
            "allReduce_1_1": {
                0: {
                    "Communication Bandwidth Info": {
                        "RDMA": {
                            "Transit Size(MB)": 1024,
                            "Transit Time(ms)": 1000,
                            "Bandwidth(GB/s)": 1,
                            "Bandwidth(Utilization)": 0.08,
                            "Large Packet Ratio": 1.0,
                            "Size Distribution": {
                               20: 2.0
                            }
                        },
                        "HCCS": {
                            "Transit Size(MB)": 1024,
                            "Transit Time(ms)": 1000,
                            "Bandwidth(GB/s)": 1,
                            "Bandwidth(Utilization)": 0.08,
                            "Large Packet Ratio": 1.0,
                            "Size Distribution": {
                                20: 2.0
                            }
                        },
                        "SDMA": {
                            "Transit Size(MB)": 1024,
                            "Transit Time(ms)": 1000,
                            "Bandwidth(GB/s)": 1,
                            "Bandwidth(Utilization)": 0,
                            "Large Packet Ratio": 0,
                            "Size Distribution": {}
                    }
                }
                }
            },
            "allReduce_1_2": {
                0: {
                    "Communication Bandwidth Info": {
                        "RDMA": {
                            "Transit Size(MB)": 1024,
                            "Transit Time(ms)": 1000,
                            "Bandwidth(GB/s)": 1,
                            "Bandwidth(Utilization)": 0.08,
                            "Large Packet Ratio": 1.0,
                            "Size Distribution": {
                                20: 2.0
                            }
                        },
                        "HCCS": {
                            "Transit Size(MB)": 1024,
                            "Transit Time(ms)": 1000,
                            "Bandwidth(GB/s)": 1,
                            "Bandwidth(Utilization)": 0.08,
                            "Large Packet Ratio": 1.0,
                            "Size Distribution": {
                                20: 2.0
                            }
                        },
                        "SDMA": {
                            "Transit Size(MB)": 1024,
                            "Transit Time(ms)": 1000,
                            "Bandwidth(GB/s)": 1,
                            "Bandwidth(Utilization)": 0,
                            "Large Packet Ratio": 0,
                            "Size Distribution": {}
                    }
                }
                }
            },
        }
        com.combine()
        ret = com.op_info
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["RDMA"]["Transit Size(MB)"], 2048)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["RDMA"]["Transit Time(ms)"], 2000)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["RDMA"]["Bandwidth(GB/s)"], 1)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["RDMA"]["Large Packet Ratio"], 1)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["RDMA"]["Bandwidth(Utilization)"],
                         1 / HcclAnalysisTool.StandardBandWidth['RDMA'])
        self.assertEqual(ret[StrConstant.TOTAL][0]
                         ["Communication Bandwidth Info"]["RDMA"]["Size Distribution"][20], 4)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["SDMA"]["Transit Size(MB)"], 2048)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["SDMA"]["Transit Time(ms)"], 2000)
        self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["SDMA"]["Bandwidth(GB/s)"], 1)