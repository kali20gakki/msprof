#!/usr/bin/python3
# coding=utf-8
"""
This script is used to parse host disk usage
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""

import logging
import os

from common_func.common import error
from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import is_number
from common_func.msvp_constant import MsvpConstant
from host_prof.host_disk_usage.model.host_disk_usage import HostDiskUsage
from host_prof.host_prof_base.host_prof_presenter_base import HostProfPresenterBase


class HostDiskUsagePresenter(HostProfPresenterBase):
    """
    class for parsing host disk usage data
    """

    FILE_NAME = os.path.basename(__file__)
    FREQ_TO_US = 10 ** 6

    def __init__(self: any, result_dir: str, file_name: str = "") -> None:
        super().__init__(result_dir, file_name)
        self.disk_usage_info = []

    @classmethod
    def get_timeline_header(cls: any) -> list:
        """
        get timeline header
        """
        return [["process_name", InfoConfReader().get_json_pid_data(),
                 InfoConfReader().get_json_tid_data(), "Disk Usage"]]

    @staticmethod
    def _get_disk_usage_items(file: any) -> tuple:
        start_time = 0
        usage_items = []

        for line in file:
            fields = line.split()
            if len(fields) < 2:  # invalid format
                continue
            if not fields[1].isdigit():  # invalid pid
                continue
            if fields[0] == 'start_time' and fields[1].isdigit():
                start_time = float(fields[1]) / NumberConstant.CONVERSION_TIME
                continue

            pid = int(fields[1])
            disk_read = fields[4] + fields[5]
            disk_write = fields[6] + fields[7]
            if line.find('unavailable') > 0 or len(fields) < 11:
                swap_in = '0.00'
                io_percent = '0.00'
            else:
                swap_in = fields[8]
                io_percent = fields[10]

            usage_items.append((pid, disk_read, disk_write,
                                swap_in, io_percent))

        return start_time, usage_items

    def init(self: any) -> None:
        """
        init class
        """
        self.set_model(HostDiskUsage(self.result_dir))

    def parse_prof_data(self: any) -> None:
        """
        parse disk usage data
        """
        try:
            with open(self.file_name, "r") as file:
                self._parse_disk_usage(file)
                logging.info(
                    "Finish parsing host disk usage data file: %s", os.path.basename(self.file_name))
        except (FileNotFoundError, ValueError, IOError) as parse_file_except:
            logging.error("Error in parsing host disk usage data:%s", str(parse_file_except),
                          exc_info=Constant.TRACE_BACK_SWITCH)

    def get_disk_usage_data(self: any) -> dict:
        """
        get disk usage data
        """
        if not self.cur_model.check_db() or not self.cur_model.has_disk_usage_data():
            self.cur_model.finalize()
            return MsvpConstant.EMPTY_DICT
        res = self.cur_model.get_disk_usage_data()
        self.cur_model.finalize()
        return res

    def get_timeline_data(self: any) -> list:
        """
        get timeline data
        """
        disk_usage_data = self.get_disk_usage_data()
        result = []
        if not disk_usage_data:
            logging.error("No disk usage data...")
            return result

        for data_item in disk_usage_data.get("data"):
            temp_data = ["Disk Usage", data_item["start"], {"Usage(%)": data_item["usage"]}]
            result.append(temp_data)
        return result

    def _parse_disk_data(self: any, start_time: float, interval: float, usage_items: any) -> None:
        end_time = start_time
        for item in usage_items:
            start_time = end_time
            end_time = start_time + interval
            disk_read = item[1]
            disk_write = item[2]
            swap_in = item[3]
            io_percent = float(item[4])
            self.disk_usage_info.append([start_time, end_time, disk_read, disk_write,
                                         swap_in, io_percent])
        self.cur_model.insert_disk_usage_data(self.disk_usage_info)

    def _parse_disk_usage(self: any, file: any) -> None:
        start_time, usage_items = HostDiskUsagePresenter._get_disk_usage_items(file)
        disk_freq = ConfigMgr.get_disk_freq(self.result_dir)
        if is_number(disk_freq) and disk_freq > 0:
            # convert freq to interval(us)
            interval = 1 / float(disk_freq) * self.FREQ_TO_US
            self._parse_disk_data(start_time, interval, usage_items)
        else:
            error(self.FILE_NAME, "Host disk freq is invalid, please check sample.json file!")
