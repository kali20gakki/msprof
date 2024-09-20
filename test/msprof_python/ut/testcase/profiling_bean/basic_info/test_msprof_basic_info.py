#!/usr/bin/python3
# -*- coding: utf-8 -*-
#  Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import json
import unittest
from unittest.mock import MagicMock

from common_func.msprof_exception import ProfException
from profiling_bean.basic_info.msprof_basic_info import BasicInfo, MsProfBasicInfo


class TestBasicInfo(unittest.TestCase):
    @staticmethod
    def test_basic_info_when_run_called_then_each_basic_run_callled():
        basic_info = BasicInfo()
        for _attr in basic_info.__dict__:
            if not hasattr(getattr(basic_info, _attr), "run"):
                continue
            setattr(getattr(basic_info, _attr), "run", MagicMock())
        basic_info.run("")
        basic_info.collection_info.run.assert_called()

    def test_MsProfBasicInfo_when_depth_exceeds_limit_then_raise_exception(self):
        json_result = {
            "key1": "value1", "key2": "value2", "key3": "value3",
        }
        info = MsProfBasicInfo("")
        try:
            info._MsProfBasicInfo__deal_with_json_keys(json_result, 6)
        except ProfException as err:
            self.assertEqual(err.code, ProfException.PROF_INVALID_DATA_ERROR)

    def test_MsProfBasicInfo_when_depth_normal_then_deal_with_key(self):
        json_result = {
            "key1": "value1", "key2": "value2", "collection_start_time": ["a"],
        }
        info = MsProfBasicInfo("")
        info._MsProfBasicInfo__deal_with_json_keys(json_result, 0)



if __name__ == '__main__':
    unittest.main()
