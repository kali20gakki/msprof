#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from common_func.constant import Constant


class SectionCalculator:

    @classmethod
    def merge_continuous_intervals(cls: any, time_section_list: list) -> list:
        result = []
        if not time_section_list:
            return result
        time_section_list = sorted(time_section_list, key=lambda x: x.start_time)
        current_section = time_section_list[0]
        for time_section in time_section_list:
            if time_section.start_time <= current_section.end_time:
                current_section.end_time = max(current_section.end_time, time_section.end_time)
            else:
                result.append(current_section)
                current_section = time_section
        result.append(current_section)
        return result

    @classmethod
    def compute_overlap_time(cls: any, master_time_section_list: list, slave_time_section_list: list) -> list:
        current_slava_key = Constant.DEFAULT_VALUE
        for master_time_section in master_time_section_list:
            overlap_time = Constant.DEFAULT_VALUE
            while current_slava_key < len(slave_time_section_list):
                if slave_time_section_list[current_slava_key].end_time <= master_time_section.start_time:
                    current_slava_key += 1
                elif slave_time_section_list[current_slava_key].start_time >= master_time_section.end_time:
                    break
                elif slave_time_section_list[current_slava_key].end_time > master_time_section.end_time:
                    overlap_time = overlap_time + (master_time_section.end_time - max(
                        slave_time_section_list[current_slava_key].start_time, master_time_section.start_time))
                    break
                else:
                    overlap_time = overlap_time + (slave_time_section_list[current_slava_key].end_time - max(
                        slave_time_section_list[current_slava_key].start_time, master_time_section.start_time))
                    current_slava_key += 1
            master_time_section.overlap_time = overlap_time
        return master_time_section_list
