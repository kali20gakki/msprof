#!/usr/bin/python3
# coding=utf-8
"""
This script is used to parse host mem usage
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""

import os
import logging
from collections import namedtuple
from decimal import Decimal

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import is_number
from common_func.msvp_constant import MsvpConstant
from common_func.utils import Utils
from host_prof.host_network_usage.model.host_network_usage import HostNetworkUsage
from host_prof.host_prof_base.host_prof_presenter_base import HostProfPresenterBase


def parse_net_stats(line: str) -> any:
    """
    parse net status by line
    """
    NetStats = namedtuple('netstats', ['intf', 'rxbytes', 'rxpackets', 'rxerrs',
                                       'txbytes', 'txpackets', 'txerrs'])
    try:
        intf, rxbytes, rxpackets, rxerrs, txbytes, txpackets, txerrs = init_params(line)
        return NetStats(intf, rxbytes, rxpackets, rxerrs, txbytes, txpackets, txerrs)
    except (TypeError, ValueError, RuntimeError) as parse_file_except:
        logging.error("Error in parsing network data:%s", str(parse_file_except),
                      exc_info=Constant.TRACE_BACK_SWITCH)
        return NetStats(0, 0, 0, 0, 0, 0, 0)
    finally:
        pass


def init_params(line: str) -> tuple:
    """
    init net status by line
    :param line:
    :return:
    """
    fields = line.split()
    intf = fields[0][0:-1]
    rxbytes = int(fields[1])
    rxpackets = int(fields[2])
    rxerrs = int(fields[3])
    txbytes = int(fields[9])
    txpackets = int(fields[10])
    txerrs = int(fields[11])
    stat_info = (intf, rxbytes, rxpackets, rxerrs, txbytes, txpackets, txerrs)
    return stat_info


class HostNetworkUsagePresenter(HostProfPresenterBase):
    """
    class for parsing host mem usage data
    """

    def __init__(self: any, result_dir: str, file_name: str = "") -> None:
        super().__init__(result_dir, file_name)
        self.network_usage_info = []
        self.speeds = {}

    def init(self: any) -> None:
        """
        init model
        """
        self.set_model(HostNetworkUsage(self.result_dir))

    def parse_prof_data(self: any) -> None:
        """
        implement interface
        """
        try:
            with open(self.file_name, "r") as file:
                self._parse_network_usage(file)
                logging.info(
                    "Finish parsing network usage data file: %s", os.path.basename(self.file_name))
        except (FileNotFoundError, ValueError, IOError) as parse_file_except:
            logging.error("Error in parsing network data:%s", str(parse_file_except),
                          exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass

    def load_interface_speed(self: any) -> None:
        """
        load speed
        """
        _, net_card = InfoConfReader().get_net_info()
        for net in net_card:
            # Mbits/sec to Byte/sec
            speed = net.get("speed")
            if is_number(speed) and speed != -1:
                self.speeds[net.get("netCardName")] = net.get("speed") * 125000

    def write_per_usage(self: any, curr_timestamp: float, curr_data: dict,
                        last_timestamp: float, last_data: dict) -> None:
        """
        caculate usage then insert to db
        """
        total_bytes = 0
        intf_num = 0
        intf = ""
        for intf, stat in curr_data.items():
            if intf not in last_data:
                continue
            rx_delta = stat.rxbytes - last_data[intf].rxbytes
            tx_delta = stat.txbytes - last_data[intf].txbytes
            if rx_delta < 0 or tx_delta < 0:
                continue
            total_bytes += rx_delta + tx_delta
            intf_num += 1

        if intf_num == 0:
            return

        avg_bytes = Decimal(total_bytes) / intf_num
        speed = avg_bytes / Decimal(float(curr_timestamp) - float(last_timestamp))
        usage = (speed * 100 / self.speeds.get(intf)).quantize(NumberConstant.USAGE_PLACES)
        self.cur_model.insert_single_data(
            [last_timestamp, curr_timestamp, str(usage)])

    def _update_cur_data(self: any, line: str, curr_data: dict) -> None:
        stat = parse_net_stats(line)
        if stat and stat.intf in self.speeds:
            curr_data[stat.intf] = stat

    def write_usage_items(self: any, file: any) -> None:
        """
        write every usage
        """
        curr_timestamp = None
        last_timestamp = None
        last_data = {}
        curr_data = {}

        for line in file:
            # timestamp
            if line.startswith('time'):
                # compute with last sample point
                if last_timestamp is not None:
                    self.write_per_usage(curr_timestamp,
                                         curr_data, last_timestamp, last_data)
                # set curr -> last
                last_data = Utils.dict_copy(curr_data)
                last_timestamp = curr_timestamp
                curr_data.clear()
                # timestamp ns to sec
                tmp_time = line.split()
                if len(tmp_time) > 1 and is_number(tmp_time[1]):
                    curr_timestamp = float(tmp_time[1]) / (10 ** 9)
            else:
                self._update_cur_data(line, curr_data)

        if curr_data:
            self.write_per_usage(curr_timestamp, curr_data,
                                 last_timestamp, last_data)

    def _parse_network_usage(self: any, file: any) -> None:
        # network intface info
        self.load_interface_speed()

        # loop all net stat sample
        self.write_usage_items(file)

    def get_network_usage_data(self: any) -> dict:
        """
        get network usage data
        """
        if not self.cur_model.check_db() or not self.cur_model.has_network_usage_data():
            self.cur_model.finalize()
            return MsvpConstant.EMPTY_DICT
        res = self.cur_model.get_network_usage_data()
        self.cur_model.finalize()
        return res

    def get_timeline_data(self: any) -> list:
        """
        get network timeline data
        """
        mem_usage_data = self.get_network_usage_data()
        result = []
        if mem_usage_data is MsvpConstant.EMPTY_DICT:
            return result

        for data in mem_usage_data.get("data"):
            temp_data = ["Network Usage", float(data["start"]) * (10 ** 6),
                         {"Usage(%)": data["usage"]}]
            result.append(temp_data)
        return result

    @classmethod
    def get_timeline_header(cls: any) -> list:
        """
        get timeline header
        """
        return [["process_name", InfoConfReader().get_json_pid_data(),
                 InfoConfReader().get_json_tid_data(), "Network Usage"]]
