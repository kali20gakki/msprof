#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

class TimerBean:

    def __init__(self, time_dict: dict):
        self.clock_realtime = time_dict.get("clock_realtime", 0)
        self.clock_realtime_start = time_dict.get("clock_realtime_start", 0)
        self.clock_realtime_end = time_dict.get("clock_realtime_end", 0)
        self.clock_monotonic_raw = time_dict.get("clock_monotonic_raw", 0)
        self.clock_monotonic_raw_start = time_dict.get("clock_monotonic_raw_start", 0)
        self.clock_monotonic_raw_end = time_dict.get("clock_monotonic_raw_end", 0)

    @property
    def host_wall(self):
        if self.clock_realtime_start:
            return int(self.clock_realtime_start) + round(
                (int(self.clock_realtime_end) - int(self.clock_realtime_start)) / 2)
        else:
            return int(self.clock_realtime)

    @property
    def host_mon(self):
        if self.clock_monotonic_raw_start:
            return int(self.clock_monotonic_raw_start) + round(
                (int(self.clock_monotonic_raw_end) - int(self.clock_monotonic_raw_start)) / 2)
        else:
            return int(self.clock_monotonic_raw)