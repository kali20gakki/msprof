#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import os
import unittest

from common_func.info_conf_reader import InfoConfReader
from constant.constant import clear_dt_project


class TestDirCRBaseModel(unittest.TestCase):
    """
    this class provide a standard sqlite dir create and delete process
    """
    DIR_PATH = os.path.join(os.path.dirname(__file__), "DT_BaseModel")
    PROF_DIR = os.path.join(DIR_PATH, 'PROF1')
    PROF_DEVICE_DIR = os.path.join(PROF_DIR, 'device')
    PROF_HOST_DIR = os.path.join(PROF_DIR, 'host')

    def setUp(self) -> None:
        clear_dt_project(self.DIR_PATH)
        path = os.path.join(self.PROF_DEVICE_DIR, 'sqlite')
        os.makedirs(path)
        path = os.path.join(self.PROF_HOST_DIR, 'sqlite')
        os.makedirs(path)
        info_reader = InfoConfReader()
        info_reader._info_json = {'platform_version': "5"}

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)
        info_reader = InfoConfReader()
        info_reader._info_json = {}
