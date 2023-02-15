#!/usr/bin/python3
# -*- coding: utf-8 -*-
#  Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import unittest

from msconfig.config_manager import ConfigManager


class TestConfigManager(unittest.TestCase):
    def test_get(self):
        config = ConfigManager.get(ConfigManager.PROF_CONDITION)
        self.assertEqual(len(config.get_data()), 27)
        config = ConfigManager.get(ConfigManager.PROF_RULE)
        self.assertEqual(len(config.get_data()), 17)
        config = ConfigManager.get(ConfigManager.DATA_PARSERS)
        self.assertEqual(len(config.sections()), 42)
        config = ConfigManager.get(ConfigManager.STARS)
        self.assertEqual(config.has_section('AA'), False)
        config = ConfigManager.get(ConfigManager.TABLES)
        self.assertEqual(len(config.options('BiuCyclesMap')), 9)
        config = ConfigManager.get(ConfigManager.TABLES_TRAINING)
        self.assertEqual(config.get('BiuCyclesMap', 'AA'), '')
        config = ConfigManager.get(ConfigManager.TABLES_OPERATOR)
        self.assertEqual(len(config.items('SummaryGeMap')), 11)
        config = ConfigManager.get(ConfigManager.MSPROF_EXPORT_DATA)
        self.assertEqual(len(config.sections()), 55)
        config = ConfigManager.get(ConfigManager.DATA_CALCULATOR)
        self.assertEqual(len(config.sections()), 21)
        config = ConfigManager.get(ConfigManager.TUNING_RULE)
        self.assertEqual(len(config.get_data()), 16)
