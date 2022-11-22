#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import os
import unittest
from unittest import mock
from tuning.cluster.cluster_tuning_facade import ClusterTuningFacade

import pytest

NAMESPACE = 'tuning.cluster.cluster_tuning_facade'


class TestClusterTuningFacade(unittest.TestCase):
    params = {"collection_path": 'test',
              "npu_id": 1,
              "model_id": 0,
              "iteration_id": 1}

    def test_cluster_communication(self):
        ClusterTuningFacade(self.params).cluster_communication()

    def test_run(self):
        with mock.patch(NAMESPACE + '.ClusterTuningFacade._check_data_type_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade._check_collection_dir_valid'), \
                mock.patch(NAMESPACE + '.ClusterTuningFacade.dispatch'):
            ClusterTuningFacade(self.params).process()

    def test_dispatch_1(self):
        ClusterTuningFacade(self.params).dispatch()

    def test_dispatch_2(self):
        self.params.setdefault('data_type', 6)
        ClusterTuningFacade(self.params).dispatch()
