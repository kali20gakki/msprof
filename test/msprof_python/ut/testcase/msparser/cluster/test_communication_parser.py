#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import unittest
from collections import defaultdict
from unittest import mock

import pytest

from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from common_func.platform.chip_manager import ChipManager
from mscalculate.hccl.hccl_task import HcclTask
from msparser.cluster.communication_parser import CommunicationParser
from msparser.cluster.meta_parser import HcclAnalysisTool
from profiling_bean.prof_enum.chip_model import ChipModel

NAMESPACE = 'msparser.cluster.communication_parser'


class Event:
    def __init__(self, hccl_name: str):
        self.hccl_name = hccl_name
        self.op_name = 'hcom_allReduce_1'
        self.size = 1000 ** 2
        self.duration = 1000000
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
        ret = HcclAnalysisTool.determine_rdma(events, 0, 5)
        self.assertEqual(ret, True)

    def test_determine_rdma2(self):
        events = [Event(StrConstant.RDMA_SEND)]
        ret = HcclAnalysisTool.determine_rdma(events, 0, 5)
        self.assertEqual(ret, False)

    def test_get_rdma_time_info(self):
        events = [Event(StrConstant.RDMA_SEND), Event(StrConstant.RDMA_SEND), Event(StrConstant.NOTIFY_WAIT),
                  Event(StrConstant.RDMA_SEND), Event(StrConstant.NOTIFY_WAIT)]
        ret = HcclAnalysisTool.get_rdma_time_info(events, 0, 5)
        self.assertEqual(ret, [1, 1])


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
        with mock.patch(NAMESPACE + '.CommunicationParser.op_bandwidth_parser'):
            hccl_data_ffts = [
                HcclTask(plane_id=0, hccl_name="0", duration=0, timestamp=0,
                         group_name="1")
            ]
            CommunicationParser({}).parse_ops({-1: hccl_data_ffts}, 'hcom_broadcast__752_0_1@9577302986659220752')
        with pytest.raises(ProfException) as err:
            CommunicationParser({}).parse_ops({1: {}}, 'name')
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_op_time_parser(self):
        # test ProfException when master_events is empty
        err_hccl_data_ffts = [
            HcclTask(plane_id=0, hccl_name="0", duration=0, timestamp=0,
                     group_name="1"),
            HcclTask(plane_id=2, hccl_name="0", duration=0, timestamp=0,
                     group_name="1")
        ]
        with pytest.raises(ProfException) as err:
            CommunicationParser({}).op_time_parser(err_hccl_data_ffts)
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)
        # test whether Idle Time(ms) = Elapse Time(ms) - Transit Time(ms) - Wait Time(ms)
        # when all event's 'is_master' is 1
        events = [
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Record', plane_id=0, timestamp=9917919035140,
                     duration=700.0, local_rank=1, remote_rank=0, transport_type='SDMA'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Wait', plane_id=0, timestamp=9917919037880,
                     duration=3813140.0, local_rank=1, remote_rank=0, transport_type='LOCAL'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Record', plane_id=1, timestamp=9917922852760,
                     duration=700.0, local_rank=1, remote_rank=0, transport_type='SDMA'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Wait', plane_id=1, timestamp=9917922856660,
                     duration=3004860.0, local_rank=1, remote_rank=0, transport_type='LOCAL'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Record', plane_id=2, timestamp=9917922853060,
                     duration=700.0, local_rank=1, remote_rank=7, transport_type='SDMA'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Wait', plane_id=2, timestamp=9917922857520,
                     duration=3006060.0, local_rank=1, remote_rank=7, transport_type='LOCAL'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Record', plane_id=3, timestamp=9917922853340,
                     duration=700.0, local_rank=1, remote_rank=6, transport_type='SDMA'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Wait', plane_id=3, timestamp=9917922858360,
                     duration=2999220.0, local_rank=1, remote_rank=6, transport_type='LOCAL'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Record', plane_id=4, timestamp=9917922853620,
                     duration=700.0, local_rank=1, remote_rank=5, transport_type='SDMA'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Wait', plane_id=4, timestamp=9917922859120,
                     duration=2994420.0, local_rank=1, remote_rank=5, transport_type='LOCAL'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Record', plane_id=5, timestamp=9917922853880,
                     duration=720.0, local_rank=1, remote_rank=4, transport_type='SDMA'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Wait', plane_id=5, timestamp=9917922859900,
                     duration=8780.0, local_rank=1, remote_rank=4, transport_type='LOCAL'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Record', plane_id=6, timestamp=9917922854160,
                     duration=700.0, local_rank=1, remote_rank=3, transport_type='SDMA'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Wait', plane_id=6, timestamp=9917922860660,
                     duration=5700.0, local_rank=1, remote_rank=3, transport_type='LOCAL'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Record', plane_id=0, timestamp=9917922854440,
                     duration=700.0, local_rank=1, remote_rank=2, transport_type='SDMA'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Wait', plane_id=0, timestamp=9917922861420,
                     duration=20.0, op_type='hcom_broadcast_', local_rank=1, remote_rank=2, transport_type='LOCAL'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Memcpy', plane_id=1, timestamp=9917925865160,
                     duration=1500.0, local_rank=1, remote_rank=0, transport_type='SDMA'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Record', plane_id=1, timestamp=9917925873160,
                     duration=700.0, local_rank=1, remote_rank=0, transport_type='SDMA'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Wait', plane_id=1, timestamp=9917925878760,
                     duration=8320.0, local_rank=1, remote_rank=0, transport_type='LOCAL'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Record', plane_id=2, timestamp=9917925866240,
                     duration=700.0, local_rank=1, remote_rank=7, transport_type='SDMA'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Wait', plane_id=2, timestamp=9917925874080,
                     duration=12220.0, local_rank=1, remote_rank=7, transport_type='LOCAL'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Record', plane_id=3, timestamp=9917925867400,
                     duration=700.0, local_rank=1, remote_rank=6, transport_type='SDMA'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Wait', plane_id=3, timestamp=9917925874940,
                     duration=8080.0, local_rank=1, remote_rank=6, transport_type='LOCAL'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Record', plane_id=4, timestamp=9917925868420,
                     duration=700.0, local_rank=1, remote_rank=5, transport_type='SDMA'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Wait', plane_id=4, timestamp=9917925875700,
                     duration=20.0, local_rank=1, remote_rank=5, transport_type='LOCAL'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Record', plane_id=5, timestamp=9917925869440,
                     duration=720.0, op_type='hcom_broadcast_', local_rank=1, remote_rank=4, transport_type='SDMA'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Wait', plane_id=5, timestamp=9917925876460,
                     duration=20.0, local_rank=1, remote_rank=4, transport_type='LOCAL'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Record', plane_id=6, timestamp=9917925870480,
                     duration=700.0, local_rank=1, remote_rank=3, transport_type='SDMA'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Wait', plane_id=6, timestamp=9917925877240,
                     duration=0.0, local_rank=1, remote_rank=3, transport_type='LOCAL'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Record', plane_id=0, timestamp=9917925871520,
                     duration=700.0, local_rank=1, remote_rank=2, transport_type='SDMA'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Notify_Wait', plane_id=0, timestamp=9917925878000,
                     duration=0.0, local_rank=1, remote_rank=2, transport_type='LOCAL'),
            HcclTask(op_name='hcom_broadcast__902_0_1', hccl_name='Memcpy', plane_id=0, timestamp=9917925888320,
                     duration=760.0, local_rank=1, remote_rank=1, transport_type='SDMA')
        ]
        op_time_dict = CommunicationParser({}).op_time_parser(events)
        self.assertAlmostEqual(op_time_dict["Transit Time(ms)"], 0.0015)
        self.assertAlmostEqual(op_time_dict["Wait Time(ms)"], 3.0131799999999997)
        self.assertAlmostEqual(op_time_dict["Idle Time(ms)"], 3.83926)
        self.assertAlmostEqual(op_time_dict["Synchronization Time(ms)"], 3.00486)
        self.assertAlmostEqual(op_time_dict["Transit Time(ms)"] + op_time_dict["Wait Time(ms)"] +
                               op_time_dict["Idle Time(ms)"], op_time_dict['Elapse Time(ms)'])

    def test_op_bandwidth_parser(self):
        standard_bandwidth = {
            StrConstant.RDMA: 12.5,
            StrConstant.HCCS: 18,
            StrConstant.PCIE: 20
        }
        expect_bandwidth_dict = {
            'HCCS': {
                'Bandwidth(GB/s)': 0,
                'Bandwidth(Utilization)': 0.0,
                'Large Packet Ratio': 0,
                'Size Distribution': defaultdict(lambda: [0, 0]),
                'Transit Size(MB)': 0,
                'Transit Time(ms)': 0
            },
            'PCIE': {
                'Bandwidth(GB/s)': 0,
                'Bandwidth(Utilization)': 0.0,
                'Large Packet Ratio': 0,
                'Size Distribution': defaultdict(lambda: [0, 0]),
                'Transit Size(MB)': 0,
                'Transit Time(ms)': 0
            },
            'SIO': {
                'Bandwidth(GB/s)': 0,
                'Bandwidth(Utilization)': 0.0,
                'Large Packet Ratio': 0,
                'Size Distribution': defaultdict(lambda: [0, 0]),
                'Transit Size(MB)': 0,
                'Transit Time(ms)': 0
            },
            'RDMA': {
                'Bandwidth(GB/s)': 1.0,
                'Bandwidth(Utilization)': 0.08,
                'Large Packet Ratio': 1.0,
                'Size Distribution': {1: [1, 1]},
                'Transit Size(MB)': 1.0,
                'Transit Time(ms)': 1.0
            },
            'SDMA': {
                'Bandwidth(GB/s)': 0,
                'Bandwidth(Utilization)': 0.0,
                'Large Packet Ratio': 0,
                'Size Distribution': defaultdict(lambda: [0, 0]),
                'Transit Size(MB)': 0,
                'Transit Time(ms)': 0
            },
        }
        events = [
            Event(StrConstant.RDMA_SEND), Event(StrConstant.RDMA_SEND), Event(StrConstant.NOTIFY_WAIT),
            Event(StrConstant.RDMA_SEND), Event(StrConstant.NOTIFY_WAIT)
        ]
        with mock.patch("msparser.cluster.meta_parser.HcclAnalysisTool.get_standard_bandwidth",
                        return_value=standard_bandwidth):
            op_bandwidth_dict = CommunicationParser({}).op_bandwidth_parser(events)
            self.assertEqual(op_bandwidth_dict, expect_bandwidth_dict)

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
        standard_bandwidth = {
            StrConstant.RDMA: 12.5,
            StrConstant.HCCS: 18,
            StrConstant.PCIE: 20
        }
        with mock.patch("msparser.cluster.meta_parser.HcclAnalysisTool.get_standard_bandwidth",
                        return_value=standard_bandwidth):
            com = CommunicationParser({})
            com.op_info = {
                "allReduce_1_1": {
                    0: {
                        "Communication Bandwidth Info": {
                            "RDMA": {
                                "Transit Size(MB)": 1000,
                                "Transit Time(ms)": 1000,
                                "Bandwidth(GB/s)": 1,
                                "Bandwidth(Utilization)": 0.08,
                                "Large Packet Ratio": 1.0,
                                "Size Distribution": {
                                    20: [2, 2]
                                }
                            },
                            "HCCS": {
                                "Transit Size(MB)": 1000,
                                "Transit Time(ms)": 1000,
                                "Bandwidth(GB/s)": 1,
                                "Bandwidth(Utilization)": 0.08,
                                "Large Packet Ratio": 1.0,
                                "Size Distribution": {
                                    20: [2, 2]
                                }
                            },
                            "SDMA": {
                                "Transit Size(MB)": 1000,
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
                                "Transit Size(MB)": 1000,
                                "Transit Time(ms)": 1000,
                                "Bandwidth(GB/s)": 1,
                                "Bandwidth(Utilization)": 0.08,
                                "Large Packet Ratio": 1.0,
                                "Size Distribution": {
                                    20: [2, 2]
                                }
                            },
                            "HCCS": {
                                "Transit Size(MB)": 1000,
                                "Transit Time(ms)": 1000,
                                "Bandwidth(GB/s)": 1,
                                "Bandwidth(Utilization)": 0.08,
                                "Large Packet Ratio": 1.0,
                                "Size Distribution": {
                                    20: [2, 2]
                                }
                            },
                            "SDMA": {
                                "Transit Size(MB)": 1000,
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
            self.assertEqual(
                ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["RDMA"]["Transit Size(MB)"], 2000
            )
            self.assertEqual(
                ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["RDMA"]["Transit Time(ms)"], 2000
            )
            self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["RDMA"]["Bandwidth(GB/s)"], 1)
            self.assertEqual(ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["RDMA"]["Large Packet Ratio"], 1)
            self.assertEqual(
                ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["RDMA"]["Bandwidth(Utilization)"],
                1 / HcclAnalysisTool.StandardBandWidth.get(ChipModel.CHIP_V2_1_0).get('RDMA')
            )
            self.assertEqual(ret[StrConstant.TOTAL][0]
                             ["Communication Bandwidth Info"]["RDMA"]["Size Distribution"][20], [4, 4])
            self.assertEqual(
                ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["SDMA"]["Transit Size(MB)"], 2000
            )
            self.assertEqual(
                ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["SDMA"]["Transit Time(ms)"], 2000
            )
            self.assertEqual(
                ret[StrConstant.TOTAL][0]["Communication Bandwidth Info"]["SDMA"]["Bandwidth(GB/s)"], 1
            )
