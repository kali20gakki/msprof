#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
import types
import unittest

from common_func.section_calculator import SectionCalculator
from common_func.msprof_object import CustomizedNamedtupleFactory
from profiling_bean.db_dto.time_section_dto import TimeSectionDto

TimeSectionDtoTuple = CustomizedNamedtupleFactory.generate_named_tuple_from_dto(TimeSectionDto, [])


class TestSectionCalculator(unittest.TestCase):

    def test_merge_continuous_intervals_when_empty_input(self):
        self.assertEqual(SectionCalculator.merge_continuous_intervals([]), [])

    def test_merge_continuous_intervals_when_continuous(self):
        section = [
            TimeSectionDtoTuple(start_time=2, end_time=4), TimeSectionDtoTuple(start_time=1, end_time=5)
        ]
        self.assertEqual(len(SectionCalculator.merge_continuous_intervals(section)), 1)

    def test_merge_continuous_intervals_when_intervals(self):
        section = [TimeSectionDtoTuple(start_time=7, end_time=9), TimeSectionDtoTuple(start_time=1, end_time=3)]
        self.assertEqual(len(SectionCalculator.merge_continuous_intervals(section)), 2)

    def test_compute_overlap_time(self):
        master_section = [
            TimeSectionDtoTuple(start_time=4, end_time=9),
            TimeSectionDtoTuple(start_time=14, end_time=15),
            TimeSectionDtoTuple(start_time=17, end_time=18)
        ]
        slave_section = [
            TimeSectionDtoTuple(start_time=1, end_time=2), TimeSectionDtoTuple(start_time=3, end_time=5),
            TimeSectionDtoTuple(start_time=6, end_time=7), TimeSectionDtoTuple(start_time=8, end_time=10),
            TimeSectionDtoTuple(start_time=11, end_time=12),
            TimeSectionDtoTuple(start_time=13, end_time=16), TimeSectionDtoTuple(start_time=19, end_time=20)
        ]
        result = SectionCalculator.compute_overlap_time(master_section, slave_section)
        self.assertEqual(result[0].overlap_time, 3)
        self.assertEqual(result[1].overlap_time, 1)
        self.assertEqual(result[2].overlap_time, 0)
