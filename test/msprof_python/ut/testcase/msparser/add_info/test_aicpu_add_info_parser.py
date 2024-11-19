#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
"""
import os
import struct
import shutil
import unittest
from unittest import mock
 
from common_func.info_conf_reader import InfoConfReader
from msparser.add_info.aicpu_add_info_bean import AicpuAddInfoBean
from msparser.add_info.aicpu_add_info_parser import AicpuAddInfoParser
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.db_dto.step_trace_dto import IterationRange
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.add_info.aicpu_add_info_parser'


class TestAicpuAddInfoParser(unittest.TestCase):
    file_list = {
        DataTag.AICPU_ADD_INFO: [
            'aicpu.data.0.slice_0'
        ]
    }
    DIR_PATH = os.path.join(os.path.dirname(__file__), "aicpu_add_info")
    SQLITE_PATH = os.path.join(DIR_PATH, "sqlite")
    CONFIG = {
        'result_dir': DIR_PATH, 'device_id': '0', 'iter_id': IterationRange(0, 1, 1),
        'job_id': 'job_default', 'model_id': -1
    }

    def setup_class(self):
        if not os.path.exists(self.DIR_PATH):
            os.mkdir(self.DIR_PATH)
        if not os.path.exists(self.SQLITE_PATH):
            os.mkdir(self.SQLITE_PATH)

    def teardown_class(self):
        if os.path.exists(self.DIR_PATH):
            shutil.rmtree(self.DIR_PATH)

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse'), \
                mock.patch(NAMESPACE + '.AicpuAddInfoParser.save'):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.ms_run()

    def test_save(self):
        check = AicpuAddInfoParser(self.file_list, self.CONFIG)
        check.save()
        check = AicpuAddInfoParser(self.file_list, self.CONFIG)
        check._ai_cpu_datas = [
            [0, '0', 1e-05, 2e-05, '', 0.0, 0.0, 1e-05, 0.0, 0.0]
        ]
        check.save()

    def test_parse_should_return_aicpu_node_data_when_type_0_and_start_time_not_0(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
        aicpu_data = (
            23130, 6000, 0, 1, 128, 2000,
            1, 2, 0, 0, 1000, 10000, 10000, 15000,
            20000, 30000, 0, 0, 40000, 40000, 40000, 40000, 2, 123, 456, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        )
        struct_data = struct.pack(StructFmt.AI_CPU_NODE_ADD_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.AICPU_NODE, [])
        self.assertEqual(1, len(data))
        self.assertEqual(0.123, data[0][8])
        InfoConfReader()._info_json = {}

    def test_parse_should_return_aicpu_node_data_when_type_0_and_start_time_0(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
        aicpu_data = (
            23130, 6000, 0, 1, 128, 2000,
            1, 2, 0, 0, 0, 10000, 10000, 15000,
            20000, 30000, 0, 0, 40000, 40000, 40000, 40000, 2, 123, 456, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        )
        struct_data = struct.pack(StructFmt.AI_CPU_NODE_ADD_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.AICPU_NODE, [])
        self.assertEqual(0, len(data))
        InfoConfReader()._info_json = {}

    def test_parse_should_return_aicpu_dp_data_when_type_1(self):
        aicpu_data = [23130, 6000, 1, 1, 128, 2000,
                      b"Last dequeue", b"mark_name_VZDO11n4aPy", 1, 56] + [0] * 17
        struct_data = struct.pack(StructFmt.AI_CPU_DP_ADD_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.AICPU_DP, [])
        self.assertEqual(1, len(data))
        self.assertEqual("Last dequeue", data[0][1])

    def test_parse_should_return_aicpu_model_data_when_type_2(self):
        aicpu_data = [23130, 6000, 2, 1, 128, 2000,
                      0, 1, 2, 0, 4] + [0] * 26
        struct_data = struct.pack(StructFmt.AI_CPU_MODEL_ADD_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.AICPU_MODEL, [])
        self.assertEqual(1, len(data))
        self.assertEqual(2, data[0][3])

    def test_parse_should_return_aicpu_mi_data_when_type_3(self):
        aicpu_data = [23130, 6000, 3, 1, 128, 2000,
                      1, 0, 10, 1000, 2000] + [0] * 25
        struct_data = struct.pack(StructFmt.AI_CPU_MI_ADD_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.AICPU_MI, [])
        self.assertEqual(1, len(data))
        self.assertEqual(2000, data[0][3])

    def test_parse_should_return_comm_turn_data_when_type_4(self):
        aicpu_data = [23130, 6000, 4, 1, 128, 20000,
                      1000, 2000, 3000, 4000, 5000, 6000, 7000, 128, 0, 1, 2, 0, 2, 0] + [0] * 43
        struct_data = struct.pack(StructFmt.KFC_COMM_TURN_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.KFC_COMM_TURN, [])
        self.assertEqual(1, len(data))
        self.assertEqual(1000, data[0][5])

    def test_parse_should_return_compute_turn_data_when_type_5(self):
        aicpu_data = [23130, 6000, 5, 1, 128, 20000,
                      1000, 2000, 3000, 128, 0, 1, 2, 0, 2, 0] + [0] * 51
        struct_data = struct.pack(StructFmt.KFC_COMPUTE_TURN_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.KFC_COMPUTE_TURN, [])
        self.assertEqual(1, len(data))
        self.assertEqual(1000, data[0][5])

    def test_parse_should_return_kfc_hccl_info_data_when_type_6(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
        aicpu_data = [23130, 6000, 6, 1, 128, 20000,
                      12345, 0, 111111, 0, 0, 8, 0, 0, 4294967295, 0, 2, 0, 0.1, 22390660833240, 22390660824120,
                      0, 0, 0, 0, 0, 10, 20] + [0] * 30
        struct_data = struct.pack(StructFmt.KFC_HCCL_INFO_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.KFC_HCCL_INFO, [])
        self.assertEqual(1, len(data))
        self.assertEqual(4294967295, data[0][9])
        InfoConfReader()._info_json = {}

    def test_parse_should_return_device_hccl_op_info_data_when_type_10(self):
        InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 100}]}
        aicpu_data = [23130, 6000, 10, 1, 128, 20000,
                      0, 0, 0, 1, 12345, 8, 6, 1] + [0] * 196
        struct_data = struct.pack(StructFmt.DEVICE_HCCL_OP_INFO_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, self.CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.HCCL_OP_INFO, [])
        self.assertEqual(1, len(data))
        self.assertEqual("INT8", data[0][3])
        InfoConfReader()._info_json = {}


if __name__ == '__main__':
    unittest.main()
