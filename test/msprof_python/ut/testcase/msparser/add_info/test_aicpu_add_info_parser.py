#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
"""
import struct
import unittest
from unittest import mock
 
from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.profiling_scene import ProfilingScene
from constant.constant import CONFIG
from msparser.add_info.aicpu_add_info_bean import AicpuAddInfoBean
from msparser.add_info.aicpu_add_info_parser import AicpuAddInfoParser
from profiling_bean.db_dto.step_trace_dto import StepTraceDto
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.add_info.aicpu_add_info_parser'


class TestAicpuAddInfoParser(unittest.TestCase):
    file_list = {
        DataTag.AICPU_ADD_INFO: [
            'aging.additional.data_preprocess.slice_0'
        ]
    }

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

    def test_parse(self):
        aicpu_data = (
            23130, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        )
        struct_data = struct.pack("2H3IQ2H7Q2I4Q3IH2B14QI", *aicpu_data)
        data = AicpuAddInfoBean.decode(struct_data)
        with mock.patch(NAMESPACE + '.AicpuAddInfoParser.parse_bean_data', return_value=[]):
            check = AicpuAddInfoParser(self.file_list, CONFIG)
            check.parse()


if __name__ == '__main__':
    unittest.main()
