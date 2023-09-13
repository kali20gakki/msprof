#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import unittest

from model.test_dir_cr_base_model import TestDirCRBaseModel
from msmodel.nano.nano_stars_model import NanoStarsModel, NanoStarsViewModel

NAMESPACE = 'msmodel.nano.nano_stars_model'

FORMAT_DATA_LIST = [
    [2, 0, 0, 'AI_CORE', 900324512280.0, 900329393820.0, 4881540.0, 1220001, 1, 530412, 0,
     382664, 244160, 415203, 333062, 0, 0, 378943, 12],
    [2, 0, 1, 'AI_CORE', 900329394020.0, 900332136660.0, 2742640.0, 685278, 1, 500064, 0,
     2552, 0, 682657, 676788, 0, 0, 7534, 4],
    [2, 0, 2, 'AI_CORE', 900332136860.0, 900396914500.0, 64777640.0, 16194028, 1, 4,
     15137216, 42922, 672, 3592381, 202059, 276320, 0, 38142, 13]
]


class TestNanoStarsModel(TestDirCRBaseModel):
    def test_flush_when_get_data_list_then_return_none(self):
        self.assertEqual(NanoStarsModel('test').flush(FORMAT_DATA_LIST), None)

    def test_get_nano_data_within_time_range_when_different_range_then_get_different_result(self):

        with NanoStarsModel(self.PROF_DEVICE_DIR) as model:
            model.flush(FORMAT_DATA_LIST)
            with NanoStarsViewModel(self.PROF_DEVICE_DIR) as view_model:
                device_tasks = view_model.get_nano_data_within_time_range(float("-inf"), float("inf"))
                self.assertEqual(len(device_tasks), 3)

                device_tasks = view_model.get_nano_data_within_time_range(0, 500)
                self.assertEqual(len(device_tasks), 0)

                device_tasks = view_model.get_nano_data_within_time_range(900324512270, 900329393830)
                self.assertEqual(len(device_tasks), 1)

    def test_get_nano_pmu_details_should_return_data_when_is_normal(self):
        with NanoStarsModel(self.PROF_DEVICE_DIR) as model:
            model.flush(FORMAT_DATA_LIST)
            with NanoStarsViewModel(self.PROF_DEVICE_DIR) as view_model:
                res_data = view_model.get_nano_pmu_details()
                self.assertEqual(len(res_data), 3)


if __name__ == '__main__':
    unittest.main()
