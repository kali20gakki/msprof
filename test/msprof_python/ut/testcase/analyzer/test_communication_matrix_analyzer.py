import os
from collections import defaultdict

import unittest
from unittest import mock
import pytest

from common_func.ms_constant.str_constant import StrConstant
from analyzer.communication_matrix_analyzer import CommunicationMatrixAnalyzer
from common_func.msprof_exception import ProfException


NAMESPACE = 'analyzer.communication_matrix_analyzer'


class HcclEvent:
    def __init__(self, hccl_name: str, timestamp: int):
        self.hccl_name = hccl_name
        self.op_name = 'hcom_allReduce__1'
        self.group_name = 'group_name'
        self.size = 1000 ** 2
        self.duration = 10
        self.plane_id = 0
        self.iteration = 0
        self.transport_type = StrConstant.RDMA
        self.timestamp = timestamp
        self.src_rank = 0
        self.dst_rank = 1


class TestCommunicationMatrixAnalyzer(unittest.TestCase):
    collection_path = 'test'

    def test_get_hccl_data_from_db_should_return_hccl_data_when_get_hccl_data_correct(self):
        events = [
            HcclEvent(StrConstant.RDMA_SEND, 0),
            HcclEvent(StrConstant.RDMA_SEND, 11),
            HcclEvent(StrConstant.NOTIFY_WAIT, 21),
            HcclEvent(StrConstant.RDMA_SEND, 31),
            HcclEvent(StrConstant.NOTIFY_WAIT, 41)
        ]
        expected_result = {
            'hcom_allReduce__1@group_name': events
        }
        with mock.patch(NAMESPACE + ".CommunicationModel.check_table", return_value=True), \
                mock.patch(NAMESPACE + ".CommunicationModel.get_all_events_from_db", return_value=events):
            analyzer = CommunicationMatrixAnalyzer(self.collection_path)
            analyzer._get_hccl_data_from_db(os.path.join(self.collection_path, "device_1"))

            self.assertEqual(analyzer.hccl_op_data, expected_result)

    def test_process_dict_should_return_correct_dict_when_process_analyzed_data(self):
        analyzed_data = [
            {
                'op_name': 'hcom_allReduce__1@group_name',
                'link_info': [
                    {
                        'Src Rank': 0,
                        'Dst Rank': 1,
                        'Transport Type': 0,
                        'Transit Size(MB)': 1,
                        'Transit Time(ms)': 1,
                        'Bandwidth(GB/s)': 1
                    },
                    {
                        'Src Rank': 0,
                        'Dst Rank': 0,
                        'Transport Type': 3,
                        'Transit Size(MB)': 1,
                        'Transit Time(ms)': 1,
                        'Bandwidth(GB/s)': 1
                    }
                ]
            },
            {
                'op_name': 'hcom_allReduce__2@group_name',
                'link_info': [
                    {
                        'Src Rank': 1,
                        'Dst Rank': 9,
                        'Transport Type': 2,
                        'Transit Size(MB)': 1,
                        'Transit Time(ms)': 1,
                        'Bandwidth(GB/s)': 1
                    },
                    {
                        'Src Rank': 1,
                        'Dst Rank': 1,
                        'Transport Type': 3,
                        'Transit Size(MB)': 1,
                        'Transit Time(ms)': 1,
                        'Bandwidth(GB/s)': 1
                    }
                ]
            }
        ]

        expected_data = {
            'hcom_allReduce__1@group_name': {
                '0-1': {
                    'Transport Type': 'HCCS',
                    'Transit Size(MB)': 1,
                    'Transit Time(ms)': 1,
                    'Bandwidth(GB/s)': 1
                },
                '0-0': {
                    'Transport Type': 'LOCAL',
                    'Transit Size(MB)': 1,
                    'Transit Time(ms)': 1,
                    'Bandwidth(GB/s)': 1
                }
            },
            'hcom_allReduce__2@group_name': {
                '1-9': {
                    'Transport Type': 'RDMA',
                    'Transit Size(MB)': 1,
                    'Transit Time(ms)': 1,
                    'Bandwidth(GB/s)': 1
                },
                '1-1': {
                    'Transport Type': 'LOCAL',
                    'Transit Size(MB)': 1,
                    'Transit Time(ms)': 1,
                    'Bandwidth(GB/s)': 1
                }
            },
        }
        analyzer = CommunicationMatrixAnalyzer(self.collection_path)
        processed_data = analyzer._process_output(analyzed_data)
        self.assertEqual(processed_data, expected_data)

    def test_generate_output_should_raise_exception_when_no_rank_hccl_data_dict(self):
        with mock.patch(NAMESPACE + ".CommunicationMatrixAnalyzer._get_hccl_data_from_db"),  \
                pytest.raises(ProfException) as err:
            analyzer = CommunicationMatrixAnalyzer(self.collection_path)
            analyzer._generate_output(os.path.join(self.collection_path, "device_1"))
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_get_hccl_data_from_db_should_raise_exception_when_no_hccl_data(self):
        with mock.patch(NAMESPACE + ".CommunicationModel.check_db", return_value=True), \
                mock.patch(NAMESPACE + ".CommunicationModel.get_all_events_from_db", return_value=[]), \
                pytest.raises(ProfException) as err:
            analyzer = CommunicationMatrixAnalyzer(self.collection_path)
            analyzer._get_hccl_data_from_db(os.path.join(self.collection_path, "device_1"))
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_get_hccl_data_from_db_should_raise_exception_when_no_hccl_allReduce_table(self):
        with mock.patch(NAMESPACE + ".CommunicationModel.check_table", return_value=False), \
                pytest.raises(ProfException) as err:
            analyzer = CommunicationMatrixAnalyzer(self.collection_path)
            analyzer._get_hccl_data_from_db(os.path.join(self.collection_path, "device_1"))
            self.assertEqual(ProfException.PROF_INVALID_CONNECT_ERROR, err.value.code)