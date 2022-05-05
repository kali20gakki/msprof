#!/usr/bin/python3
# coding=utf-8
"""
This script is used to parse host mem usage
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""
import os
import logging
from decimal import Decimal

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import is_number
from common_func.msvp_constant import MsvpConstant
from host_prof.host_mem_usage.model.host_mem_usage import HostMemUsage
from host_prof.host_prof_base.host_prof_presenter_base import HostProfPresenterBase


class HostMemUsagePresenter(HostProfPresenterBase):
    """
    class for parsing host mem usage data
    """

    def __init__(self: any, result_dir: str, file_name: str = "") -> None:
        super().__init__(result_dir, file_name)
        self.mem_usage_info = []

    def init(self: any) -> None:
        """
        init class
        """
        self.set_model(HostMemUsage(self.result_dir))

    def _parse_mem_data(self: any, file: any, mem_total: str) -> None:
        if not mem_total or not is_number(mem_total):
            logging.error("mem_total is invalid, please check file info.json...")
            return
        last_timestamp = None

        line = file.readline()
        while line:
            usage_detail = line.split()
            # parse data from usage_detail
            curr_timestamp = usage_detail[0]
            # The used memory is four times the physical memory
            usage_data = Decimal(usage_detail[2]) * 4 / mem_total * NumberConstant.PERCENTAGE
            usage = str(usage_data.quantize(NumberConstant.USAGE_PLACES))

            if last_timestamp is not None:
                item = [last_timestamp, curr_timestamp, usage]
                self.mem_usage_info.append(item)

            last_timestamp = curr_timestamp
            line = file.readline()

        self.cur_model.insert_mem_usage_data(self.mem_usage_info)

    def _parse_mem_usage(self: any, file: any) -> None:
        mem_total = InfoConfReader().get_mem_total()
        self._parse_mem_data(file, mem_total)

    def parse_prof_data(self: any) -> None:
        """
        parse prof data
        """
        try:
            with open(self.file_name, "r") as file:
                self._parse_mem_usage(file)
                logging.info(
                    "Finish parsing host mem usage data file: %s", os.path.basename(self.file_name))
        except (FileNotFoundError, ValueError, IOError) as parse_file_except:
            logging.error("Error in parsing host mem usage data:%s", str(parse_file_except),
                          exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass

    def get_mem_usage_data(self: any) -> dict:
        """
        return mem usage data
        """
        if not self.cur_model.check_db() or not self.cur_model.has_mem_usage_data():
            self.cur_model.finalize()
            return MsvpConstant.EMPTY_DICT
        res = self.cur_model.get_mem_usage_data()
        self.cur_model.finalize()
        return res

    def get_timeline_data(self: any) -> list:
        """
        get timline data
        """
        mem_usage_data = self.get_mem_usage_data()
        result = []
        if not mem_usage_data:
            logging.error("Mem usage data is empty, please check if it exists!")
            return result
        for data_item in mem_usage_data.get("data"):
            if is_number(data_item["start"]):
                temp_data = ["Memory Usage", float(data_item["start"]) / NumberConstant.CONVERSION_TIME,
                             {"Usage(%)": data_item["usage"]}]
                result.append(temp_data)
        return result

    @classmethod
    def get_timeline_header(cls: any) -> list:
        """
        get mem timeline header
        """
        return [["process_name", InfoConfReader().get_json_pid_data(),
                 InfoConfReader().get_json_tid_data(), "Memory Usage"]]
