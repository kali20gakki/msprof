#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.add_info.graph_add_info_model import GraphAddInfoModel
from msmodel.nano.nano_exeom_model import NanoExeomModel
from msmodel.nano.nano_exeom_model import NanoExeomViewModel
from msmodel.nano.nano_exeom_model import NanoGraphAddInfoViewModel

NAMESPACE = 'msmodel.nano.nano_exeom_model'

ADD_INFO_DATA_LIST = [['model', 'model_exeom', 844, 187587417144680, 'quantized', 2]]
GE_TASK_INFO_DATA_LIST = [
    [2, 'trans_TransData_0', 0, 0, 1, 0, '0', 'KERNEL', 'TransData', -1, 16, -1, 0, -1,
     'FLOAT16;', '32,1,512,36;', 'NHWC;', 'FLOAT16;', '32,5,1,512,8;', 'FORMAT_1;', 0, 4294967295, "NO"]
]


class TestNanoExeomModel(TestDirCRBaseModel):
    def test_flush_when_get_data_list_then_return_none(self):
        with mock.patch(NAMESPACE + '.NanoExeomModel.insert_data_to_db'):
            self.assertEqual(NanoExeomModel('test').flush(GE_TASK_INFO_DATA_LIST,
                                                          table_name=DBNameConstant.TABLE_GE_TASK), None)

    def test_get_nano_model_filename_with_model_id_data_then_get_name_with_id(self):
        with GraphAddInfoModel(self.PROF_DEVICE_DIR) as model:
            model.flush(ADD_INFO_DATA_LIST)
            with NanoGraphAddInfoViewModel(self.PROF_DEVICE_DIR) as view_model:
                name_dict = {'quantized': 2}
                res = view_model.get_nano_model_filename_with_model_id_data()
                self.assertEqual(res, name_dict)

    def test_get_nano_thread_id_with_model_id_data_then_get_model_id_with_thread_id(self):
        with GraphAddInfoModel(self.PROF_DEVICE_DIR) as model:
            model.flush(ADD_INFO_DATA_LIST)
            with NanoGraphAddInfoViewModel(self.PROF_DEVICE_DIR) as view_model:
                thread_id_dict = {2: 844}
                res = view_model.get_nano_thread_id_with_model_id_data()
                self.assertEqual(res, thread_id_dict)

    def test_get_nano_host_data_then_get_host_info(self):
        with NanoExeomModel(self.PROF_DEVICE_DIR) as model:
            model.flush(GE_TASK_INFO_DATA_LIST, DBNameConstant.TABLE_GE_TASK)
            with NanoExeomViewModel(self.PROF_DEVICE_DIR) as view_model:
                host_info_dict = {(2, 0): [(0, 1)]}
                res = view_model.get_nano_host_data()
                self.assertEqual(res, host_info_dict)


if __name__ == '__main__':
    unittest.main()
