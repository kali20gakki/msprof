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
from constant.constant import CONFIG
from msparser.add_info.aicpu_add_info_bean import AicpuAddInfoBean
from msparser.add_info.aicpu_add_info_parser import AicpuAddInfoParser
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.add_info.aicpu_add_info_parser'


class TestAicpuAddInfoParser(unittest.TestCase):
    file_list = {
        DataTag.AICPU_ADD_INFO: [
            'aicpu.data.0.slice_0'
        ]
    }
    current_path = CONFIG.get('result_dir')
    file_folder = os.path.join(current_path + "/sqlite")

    def setup_class(self):
        if not os.path.exists(self.current_path):
            os.mkdir(self.current_path)
        if not os.path.exists(self.file_folder):
            os.mkdir(self.file_folder)

    def teardown_class(self):
        if os.path.exists(self.current_path):
            shutil.rmtree(self.current_path)

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse'), \
                mock.patch(NAMESPACE + '.AicpuAddInfoParser.save'):
            check = AicpuAddInfoParser(self.file_list, CONFIG)
            check.ms_run()

    def test_save(self):
        check = AicpuAddInfoParser(self.file_list, CONFIG)
        check.save()
        check = AicpuAddInfoParser(self.file_list, CONFIG)
        check._ai_cpu_datas = [
            [0, '0', 1e-05, 2e-05, '', 0.0, 0.0, 1e-05, 0.0, 0.0]
        ]
        check.save()

    def test_parse_should_return_aicpu_node_data_when_type_0(self):
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
            check = AicpuAddInfoParser(self.file_list, CONFIG)
            check.parse()
            check.save()
        data = check._aicpu_data.get(AicpuAddInfoBean.AICPU_NODE, [])
        self.assertEqual(1, len(data))
        self.assertEqual(0.123, data[0][8])
        InfoConfReader()._info_json = {}

    def test_parse_should_return_aicpu_dp_data_when_type_1(self):
        aicpu_data = [23130, 6000, 1, 1, 128, 2000,
                      b"Last dequeue", b"mark_name_VZDO11n4aPy", 1, 56] + [0] * 17
        struct_data = struct.pack(StructFmt.AI_CPU_DP_ADD_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, CONFIG)
            check.parse()
        data = check._aicpu_data.get(AicpuAddInfoBean.AICPU_DP, [])
        self.assertEqual(1, len(data))
        self.assertEqual("Last dequeue", data[0][1])

    def test_parse_should_return_aicpu_model_data_when_type_2(self):
        aicpu_data = [23130, 6000, 2, 1, 128, 2000,
                      0, 1, 2, 0, 4] + [0] * 26
        struct_data = struct.pack(StructFmt.AI_CPU_MODEL_ADD_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, CONFIG)
            check.parse()
        data = check._aicpu_data.get(AicpuAddInfoBean.AICPU_MODEL, [])
        self.assertEqual(1, len(data))
        self.assertEqual(2, data[0][3])

    def test_parse_should_return_aicpu_mi_data_when_type_3(self):
        aicpu_data = [23130, 6000, 3, 1, 128, 2000,
                      1, 0, 10, 1000, 2000] + [0] * 25
        struct_data = struct.pack(StructFmt.AI_CPU_MI_ADD_FMT, *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[data]):
            check = AicpuAddInfoParser(self.file_list, CONFIG)
            check.parse()
        data = check._aicpu_data.get(AicpuAddInfoBean.AICPU_MI, [])
        self.assertEqual(1, len(data))
        self.assertEqual(2000, data[0][3])


if __name__ == '__main__':
    unittest.main()
