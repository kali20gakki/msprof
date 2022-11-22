#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import os
import unittest
from unittest import mock

from constant.constant import clear_dt_project

NAMESPACE = 'msparser.cluster.cluster_communication_parser'


class TestClusterCommunicationParser(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_ClusterCommunicationParser')
    param = {
        "collection_path": DIR_PATH,
        "is_cluster": True,
        "npu_id": -1,
        "model_id": 1,
        "iteration_id": 1
    }

    def setUp(self) -> None:
        os.mkdir(self.DIR_PATH)

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)


