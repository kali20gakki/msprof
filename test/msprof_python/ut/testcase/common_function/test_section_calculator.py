#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import unittest

from common_func.section_calculator import SectionCalculator
from profiling_bean.db_dto.time_section_dto import TimeSectionDto


class TestSectionCalculator(unittest.TestCase):

    def test_merge_continuous_intervals_when_empty_input(self):
        self.assertEqual(SectionCalculator.merge_continuous_intervals([]), [])

    def test_merge_continuous_intervals_when_continuous(self):
        section = [TimeSectionDto(), TimeSectionDto()]
        section[0].start_time = 2
        section[0].end_time = 4
        section[1].start_time = 1
        section[1].end_time = 5
        self.assertEqual(len(SectionCalculator.merge_continuous_intervals(section)), 1)

    def test_merge_continuous_intervals_when_intervals(self):
        section = [TimeSectionDto(), TimeSectionDto()]
        section[0].start_time = 7
        section[0].end_time = 9
        section[1].start_time = 1
        section[1].end_time = 3
        self.assertEqual(len(SectionCalculator.merge_continuous_intervals(section)), 2)

    def test_compute_overlap_time(self):
        master_section = [TimeSectionDto(), TimeSectionDto(), TimeSectionDto()]
        slave_section = [TimeSectionDto(), TimeSectionDto(), TimeSectionDto(), TimeSectionDto(), TimeSectionDto(),
                         TimeSectionDto(), TimeSectionDto()]
        master_section[0].start_time = 4
        master_section[0].end_time = 9
        master_section[1].start_time = 14
        master_section[1].end_time = 15
        master_section[2].start_time = 17
        master_section[2].end_time = 18
        slave_section[0].start_time = 1
        slave_section[0].end_time = 2
        slave_section[1].start_time = 3
        slave_section[1].end_time = 5
        slave_section[2].start_time = 6
        slave_section[2].end_time = 7
        slave_section[3].start_time = 8
        slave_section[3].end_time = 10
        slave_section[4].start_time = 11
        slave_section[4].end_time = 12
        slave_section[5].start_time = 13
        slave_section[5].end_time = 16
        slave_section[6].start_time = 19
        slave_section[6].end_time = 20
        result = SectionCalculator.compute_overlap_time(master_section, slave_section)
        self.assertEqual(result[0].overlap_time, 3)
        self.assertEqual(result[1].overlap_time, 1)
        self.assertEqual(result[2].overlap_time, 0)
