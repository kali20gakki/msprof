#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest

from model.interface.ianalysis_model import IAnalysisModel


class UTAnalysis(IAnalysisModel):

    def get_timeline_data(self):
        pass

    def get_summary_data(self):
        pass


class TestInanlysis(unittest.TestCase):
    def test_ianalysis(self):
        check = UTAnalysis()
        check.get_timeline_data()
        check.get_summary_data()
