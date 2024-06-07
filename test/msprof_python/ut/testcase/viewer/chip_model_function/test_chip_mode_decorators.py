#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
import unittest
from unittest import mock
from profiling_bean.prof_enum.chip_model import ChipModel
from viewer.chip_model_function.chip_model_decorators import ChipModeDecorators

header = [
    'Model Name', 'Model ID', 'Task ID', 'Stream ID', 'Infer ID', 'Op Name', 'OP Type', 'OP State', 'Task Type',
    'Task Start Time(us)', 'Task Duration(us)', 'Task Wait Time(us)', 'Block Dim', 'Mix Block Dim',
    'HF32 Eligible', 'Input Shapes', 'Input Data Types', 'Input Formats', 'Output Shapes', 'Output Data Types',
    'Output Formats', 'Context ID', 'total_time', 'total_cycles', 'ub_read_bw', 'ub_write_bw', 'l1_read_bw',
    'l1_write_bw', 'main_mem_read_bw', 'main_mem_write_bw'
]

tasks = [
    [
            'segformer_framework_onnx_aipp_0_batch_1_input_fp16_output_int32', 1, 0, 3, 1, 'trans_Cast_0', 'Cast',
            'static', 'AI_VECTOR_CORE', 13126617660075.5, 196916.666015625, 0, 1, 0, 'NO', '"1,3,512,1024"', 'FLOAT',
            'NCHW', '"1,3,512,1024"', 'FLOAT16', 'NCHW', 'N/A', 190.18, 237729, 0.0, 0.0, 0.0, 0.0, 0.482, 0.243
    ]
]

res = [
    [
        'segformer_framework_onnx_aipp_0_batch_1_input_fp16_output_int32', 1, 0, 3, 1, 'trans_Cast_0', 'Cast',
        'static', 'AI_VECTOR_CORE', 13126617660075.5, 196916.666015625, 0, 1, 0, 'NO', '"1,3,512,1024"', 'FLOAT',
        'NCHW', '"1,3,512,1024"', 'FLOAT16', 'NCHW', 'N/A', 190.18, 237729, 'N/A', 'N/A', 0.0, 0.0, 0.482, 'N/A'
     ]
]


class TestChipModeDecorators(unittest.TestCase):

    @classmethod
    @ChipModeDecorators.pmu_format_for_chip_model
    def pmu_data_report(cls, arg_header, arg_task):
        return arg_task

    def test_pmu_format_for_chip_model(self):
        with mock.patch('viewer.chip_model_function.chip_model_decorators.get_disable_support_pmu_set',
                        return_value={'ub_read_bw', 'ub_write_bw', 'main_mem_write_bw'}):
            ret = self.pmu_data_report(header, tasks)
            self.assertEqual(res, ret)


if __name__ == '__main__':
    unittest.main()
