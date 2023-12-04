#!/usr/bin/python3
# -*- coding: utf-8 -*-
#  Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import unittest
from unittest.mock import MagicMock

from profiling_bean.basic_info.msprof_basic_info import BasicInfo


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


if __name__ == '__main__':
    unittest.main()
