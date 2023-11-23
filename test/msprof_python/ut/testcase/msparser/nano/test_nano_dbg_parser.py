#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import struct
import unittest
from unittest import mock

import pytest

from common_func.msprof_exception import ProfException
from constant.constant import CONFIG
from msparser.nano.nano_dbg_parser import NanoDbgParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.nano.nano_dbg_parser'

GE_TASK_INFO_DATA_LIST = [
    [2, 'trans_TransData_0', 0, 0, 1, 0, '0', 'KERNEL', 'TransData', -1, 16, -1, 0, -1,
     'NHWC;', 'FLOAT16;', '32,1,512,36;', 'FORMAT_1;', 'FLOAT16;', '32,5,1,512,8;', 0, 4294967295, 'NO']
]
HOST_TASK_INFO_DATA_LIST = [[2, 0, 0, 0, '4294967295', 0, 'KERNEL', 0, -1, -1]]


class TestNanoDbgParser(unittest.TestCase):
    file_list = {DataTag.DBG_FILE: ['quantized.dbg']}

    def test_ms_run_when_error_then_failed(self):
        with mock.patch(NAMESPACE + '.logging.error', return_value=True):
            check = NanoDbgParser({DataTag.DBG_FILE: []}, CONFIG)
            check.ms_run()
        with mock.patch(NAMESPACE + '.NanoDbgParser.parse', side_effect=SystemError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = NanoDbgParser(self.file_list, CONFIG)
            check.ms_run()

    def test_ms_run_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.NanoDbgParser.parse'), \
                mock.patch(NAMESPACE + '.NanoDbgParser.save'):
            check = NanoDbgParser(self.file_list, CONFIG)
            check.ms_run()

    def test_parse_when_normal_then_pass(self):
        name_dict = {'quantized': 2}
        with mock.patch(NAMESPACE + '.NanoDbgParser._get_nano_model_filename_with_id_data', return_value=name_dict), \
                mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.NanoDbgParser.get_file_path_and_check', return_value=True), \
                mock.patch('common_func.file_manager.check_path_valid', return_value=True), \
                mock.patch(NAMESPACE + '.logging.info', return_value=True):
            with mock.patch('os.path.getsize', return_value=8000), \
                    mock.patch('builtins.open', mock.mock_open(read_data=b"\0x00\0x00")), \
                    mock.patch(NAMESPACE + '.NanoDbgParser._check_magic_num'), \
                    mock.patch(NAMESPACE + '.NanoDbgParser._get_task_info'):
                check = NanoDbgParser(self.file_list, CONFIG)
                check.parse()

    def test_save_when_no_data_then_error(self):
        check = NanoDbgParser(self.file_list, CONFIG)
        check.save()

    def test_save_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + ".NanoDbgParser._reformat_ge_task_data", return_value=GE_TASK_INFO_DATA_LIST), \
                mock.patch('msmodel.nano.nano_exeom_model.NanoExeomModel.flush'), \
                mock.patch(NAMESPACE + ".NanoDbgParser._reformat_ge_task_data",
                           return_value=HOST_TASK_INFO_DATA_LIST), \
                mock.patch('msmodel.runtime.runtime_host_task_model.RuntimeHostTaskModel.create_table'), \
                mock.patch('msmodel.runtime.runtime_host_task_model.RuntimeHostTaskModel.flush'):
            check = NanoDbgParser(self.file_list, CONFIG)
            check.save()

    def test_GE_Task_reformat_task_data_when_get_data_list_then_return_format_data(self):
        dict_list = [
            {'task_id': 0, 'stream_id': 0, 'task_type': 'KERNEL', 'block_dim': 1,
             'op_name': 'trans_TransData_0', 'op_type': 'TransData', 'input_data_type': 'FLOAT16;',
             'input_format': 'NHWC;', 'input_shape': '32,1,512,36;', 'output_data_type': 'FLOAT16;',
             'output_format': 'FORMAT_1;', 'output_shape': '32,5,1,512,8;', 'model_id': 2}
        ]
        thread_id_dict = {2: 16}
        with mock.patch(NAMESPACE + '.NanoGraphAddInfoViewModel.get_nano_thread_id_with_model_id_data',
                        return_value=thread_id_dict):
            self.assertEqual(NanoDbgParser(self.file_list, CONFIG)._reformat_ge_task_data(dict_list),
                             GE_TASK_INFO_DATA_LIST)

    def test_get_op_name_when_exist_l1_sub_graph_no_then_return_original_name(self):
        dict_list = {
            'task_id': 0, 'stream_id': 0, 'task_type': 'KERNEL', 'block_dim': 1,
            'op_name': 'trans_TransData_0', 'op_type': 'TransData', 'original_name': 'original_name',
            'l1_sub_graph_no': 'exist', 'input_data_type': 'FLOAT16;', 'input_format': 'NHWC;',
            'input_shape': '32,1,512,36;', 'output_data_type': 'FLOAT16;', 'output_format': 'FORMAT_1;',
            'output_shape': '32,5,1,512,8;', 'model_id': 2
        }
        self.assertEqual(NanoDbgParser(self.file_list, CONFIG)._get_op_name(dict_list), 'original_name')

    def test_get_op_name_when_no_l1_sub_graph_no_then_return_op_name(self):
        dict_list = {
            'task_id': 0, 'stream_id': 0, 'task_type': 'KERNEL', 'block_dim': 1,
            'op_name': 'trans_TransData_0', 'op_type': 'TransData', 'input_data_type': 'FLOAT16;',
            'input_format': 'NHWC;', 'input_shape': '32,1,512,36;', 'output_data_type': 'FLOAT16;',
            'output_format': 'FORMAT_1;', 'output_shape': '32,5,1,512,8;', 'model_id': 2
        }
        self.assertEqual(NanoDbgParser(self.file_list, CONFIG)._get_op_name(dict_list), 'trans_TransData_0')

    def test_Host_Task_reformat_task_data_when_get_data_list_then_return_format_data(self):
        dict_list = [
            {'task_id': 0, 'stream_id': 0, 'task_type': 'KERNEL', 'block_dim': 1,
             'op_name': 'trans_TransData_0', 'op_type': 'TransData', 'input_data_type': 'FLOAT16;',
             'input_format': 'NHWC;', 'input_shape': '32,1,512,36;', 'output_data_type': 'FLOAT16;',
             'output_format': 'FORMAT_1;', 'output_shape': '32,5,1,512,8;', 'model_id': 2}
        ]
        self.assertEqual(NanoDbgParser(self.file_list, CONFIG)._reformat_host_task_data(dict_list),
                         HOST_TASK_INFO_DATA_LIST)

    def test_check_magic_num_when_normal_then_pass(self):
        normal_data = (0, 1515870810)
        magic_data = struct.pack("=II", *normal_data)
        data_offset = 0
        check = NanoDbgParser(self.file_list, CONFIG)
        check._check_magic_num(magic_data, data_offset)

    def test_check_magic_num_when_error_then_failed(self):
        error_data = (0, 1515870811)
        magic_data = struct.pack("=II", *error_data)
        data_offset = 0
        with pytest.raises(ProfException) as error:
            check = NanoDbgParser(self.file_list, CONFIG)
            check._check_magic_num(magic_data, data_offset)
            self.assertEqual(error.value, 8)

    def test_get_task_info_when_normal_then_get_data(self):
        byte_data = (0, 0, 1, 37, 1, 0, 0, 0, 0, 1, 0, 404)
        task_data = struct.pack("=IIIIIIIIIIBI", *byte_data)
        with mock.patch(NAMESPACE + '.NanoDbgParser._get_op_info', return_value=45):
            check = NanoDbgParser(self.file_list, CONFIG)
            check._get_task_info(task_data, 0, 45, 2)
            dict_data = getattr(check, '_model_info')[0]
            self.assertEqual(dict_data.get("task_id"), 0)
            self.assertEqual(dict_data.get("stream_id"), 0)
            self.assertEqual(dict_data.get("task_type"), 'AI_CORE')
            self.assertEqual(dict_data.get("block_dim"), 1)
            self.assertEqual(dict_data.get("model_id"), 2)

    def test_get_task_info_when_task_type_invalid_then_get_data(self):
        byte_data = (0, 0, 1, 37, 1, 0, 0, 0, 11, 1, 0, 404)
        task_data = struct.pack("=IIIIIIIIIIBI", *byte_data)
        with mock.patch(NAMESPACE + '.NanoDbgParser._get_op_info', return_value=45):
            check = NanoDbgParser(self.file_list, CONFIG)
            check._get_task_info(task_data, 0, 45, 2)
            dict_data = getattr(check, '_model_info')[0]
            self.assertEqual(dict_data.get("task_id"), 0)
            self.assertEqual(dict_data.get("stream_id"), 0)
            self.assertEqual(dict_data.get("task_type"), '11')
            self.assertEqual(dict_data.get("block_dim"), 1)
            self.assertEqual(dict_data.get("model_id"), 2)

    def test_get_op_info_when_normal_then_get_data(self):
        byte_data = (
            0, 1, 'q', 1, 1, 'w', 2, 1, 'e', 3, 1, 'r',
            4, 10, 5, 10, 6, 10
        )
        op_data = struct.pack("=II1sII1sII1sII1sIIIIII",
                              *byte_data[:2], byte_data[2].encode(),
                              *byte_data[3:5], byte_data[5].encode(),
                              *byte_data[6:8], byte_data[8].encode(),
                              *byte_data[9:11], byte_data[11].encode(),
                              *byte_data[12:])
        with mock.patch(NAMESPACE + '.NanoDbgParser._get_format_and_type', return_value=52):
            check = NanoDbgParser(self.file_list, CONFIG)
            res = {}
            self.assertEqual(check._get_op_info(op_data, res, 0, 60), 70)
            self.assertEqual(res.get("op_name"), "q")
            self.assertEqual(res.get("op_type"), "w")
            self.assertEqual(res.get("original_name"), "e")
            self.assertEqual(res.get("l1_sub_graph_no"), "r")

    def test_get_format_and_type_when_input_normal_then_get_data(self):
        byte_data = (1, 1, 1, 3, 0, 0, 1179648, 80)
        input_data = struct.pack("=IIIIQQQI", *byte_data)
        with mock.patch(NAMESPACE + '.NanoDbgParser._get_shape'):
            check = NanoDbgParser(self.file_list, CONFIG)
            res = {}
            self.assertEqual(check._get_format_and_type(input_data, res, 0, 4), 124)
            self.assertEqual(res.get("input_data_type"), "FLOAT16")
            self.assertEqual(res.get("input_format"), "NHWC")

    def test_get_format_and_type_when_input_invalid_then_get_data(self):
        byte_data = (1, 123456, 67108893, 3, 0, 0, 1179648, 80)
        input_data = struct.pack("=IIIIQQQI", *byte_data)
        with mock.patch(NAMESPACE + '.NanoDbgParser._get_shape'):
            check = NanoDbgParser(self.file_list, CONFIG)
            res = {}
            self.assertEqual(check._get_format_and_type(input_data, res, 0, 4), 124)
            self.assertEqual(res.get("input_data_type"), "123456")
            self.assertEqual(res.get("input_format"), "67108893")

    def test_get_format_and_type_when_output_normal_then_get_data(self):
        byte_data = (1, 1, 1, 5, 0, 28, 1, 0, 0, 1310720, 110)
        output_data = struct.pack("=IIIIIIIQQQI", *byte_data)
        with mock.patch(NAMESPACE + '.NanoDbgParser._get_shape'):
            check = NanoDbgParser(self.file_list, CONFIG)
            res = {}
            self.assertEqual(check._get_format_and_type(output_data, res, 0, 5), 166)
            self.assertEqual(res.get("output_data_type"), "FLOAT16")
            self.assertEqual(res.get("output_format"), "NHWC")

    def test_get_format_and_type_when_output_invalid_then_get_data(self):
        byte_data = (1, 123456, 67108867, 5, 0, 28, 1, 0, 0, 1310720, 110)
        output_data = struct.pack("=IIIIIIIQQQI", *byte_data)
        with mock.patch(NAMESPACE + '.NanoDbgParser._get_shape'):
            check = NanoDbgParser(self.file_list, CONFIG)
            res = {}
            self.assertEqual(check._get_format_and_type(output_data, res, 0, 5), 166)
            self.assertEqual(res.get("output_data_type"), "123456")
            self.assertEqual(res.get("output_format"), "67108867")

    def test_get_shape_when_normal_then_get_data(self):
        byte_data = (0, 32, 32, 1, 512, 36)
        shape_data = struct.pack("=IIQQQQ", *byte_data)
        check = NanoDbgParser(self.file_list, CONFIG)
        res = {}
        check._get_shape(shape_data, res, 0, 40, "all")
        self.assertEqual(res.get("all_shape"), '32,1,512,36')


if __name__ == '__main__':
    unittest.main()
