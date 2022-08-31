#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
from argparse import Namespace
from unittest import mock

from msinterface.msprof_entrance import MsprofEntrance

NAMESPACE = 'msinterface.msprof_entrance'


class TestMsprofEntrance:

    def test_main(self):
        args = ["msprof_entrance.py", "query", "-dir", "test"]
        collection_path = {"collection_path": "test"}
        args_co = Namespace(**collection_path)
        with mock.patch('sys.argv', args), \
                mock.patch('argparse.ArgumentParser.parse_args', return_value=args_co), \
                mock.patch(NAMESPACE + '.QueryCommand.process'), \
                mock.patch('sys.exit'):
            MsprofEntrance().main()
