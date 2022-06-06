# coding:utf-8
"""
This script is used to provide lowpower sample transmission reports
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""

from abc import ABC

from common_func.info_conf_reader import InfoConfReader
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from model.stars.inter_soc_model import InterSocModel
from model.stars.lowpower_model import LowPowerModel
from viewer.interface.base_viewer import BaseViewer


class LowPowerViewer(BaseViewer, ABC):
    """
    class for get lowpower samplen data
    """

    DATA_TYPE = 'data_type'
    TIME_STAMP = 0
    SAMPLEING_TIMES = 1
    TEM_OF_AI_CORE = 2
    TEM_OF_HBM = 3
    TEM_OF_HBM_GRANULARITY = 4
    TEM_OF_CPU = 6
    TEM_OF_3DSRAM = 5
    TEM_OF_PERIPHERALS = 7
    TEM_OF_L2_BUFF = 8
    AIC_CURRENT_DPM = 9
    POWER_COS_DPM = 10
    AIC_CURRENT_SD5003 = 11
    POWER_COS_SD5003 = 12
    AIC_FREQUENCY = 14
    IMON = 13
    WARN_CNT0 = 15
    WARN_CNT1 = 16
    WARN_CNT2 = 17
    WARN_CNT3 = 18
    VOLTAGE = 29

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.pid = 0
        self.model_list = {
            'inter_soc_time': InterSocModel,
            'inter_soc_transmission': InterSocModel,
            'low_power': LowPowerModel,
        }

    def get_timeline_header(self: any) -> list:
        """
        get timeline trace header
        :return: list
        """
        low_power_header = [["process_name", self.pid,
                             InfoConfReader().get_json_tid_data(), self.params.get(self.DATA_TYPE)]]
        return low_power_header

    def get_trace_timeline(self: any, datas: list) -> list:
        """
        format data to standard timeline format
        :return: list
        """
        if not datas:
            return []
        result = []
        for data in datas:
            result.append(["Sampling Times", data[self.TIME_STAMP], self.pid, self.SAMPLEING_TIMES,
                           {'Value': data[self.SAMPLEING_TIMES]}])
            result.append(["AIC TEMP (℃)", data[self.TIME_STAMP], self.pid, self.TEM_OF_AI_CORE,
                           {'Value': data[self.TEM_OF_AI_CORE]}])
            result.append(
                ["HBM Controller TEMP (℃)", data[self.TIME_STAMP], self.pid, self.TEM_OF_HBM,
                 {'Value': data[self.TEM_OF_HBM]}])
            result.append(["HBM TEMP (℃)", data[self.TIME_STAMP], self.pid, self.TEM_OF_HBM,
                           {'Value': data[self.TEM_OF_HBM_GRANULARITY]}])
            result.append(
                ["CPU TEMP (℃)", data[self.TIME_STAMP], self.pid, self.TEM_OF_CPU,
                 {'Value': data[self.TEM_OF_CPU]}])
            result.append(["Peripherals TEMP (℃)", data[self.TIME_STAMP], self.pid, self.TEM_OF_PERIPHERALS,
                           {'Value': data[self.TEM_OF_PERIPHERALS]}])
            result.append(["L2 Buffer TEMP (℃)", data[self.TIME_STAMP], self.pid, self.TEM_OF_L2_BUFF,
                           {'Value': data[self.TEM_OF_L2_BUFF]}])
            result.append(["DPM AIC Current (A)", data[self.TIME_STAMP], self.pid, self.AIC_CURRENT_DPM,
                           {'Value': data[self.AIC_CURRENT_DPM]}])
            result.append(["DPM Power (W)", data[self.TIME_STAMP], self.pid, self.POWER_COS_DPM,
                           {'Value': data[self.POWER_COS_DPM]}])
            result.append(["AIC Current (A)", data[self.TIME_STAMP], self.pid, self.AIC_CURRENT_SD5003,
                           {'Value': data[self.AIC_CURRENT_SD5003]}])
            result.append(["AIC Power (W)", data[self.TIME_STAMP], self.pid, self.POWER_COS_SD5003,
                           {'Value': data[self.POWER_COS_SD5003]}])
            result.append(
                ["AIC Voltage (V)", data[self.TIME_STAMP], self.pid, self.VOLTAGE, {'Value': data[self.VOLTAGE]}])
            result.append(["AIC Frequency (MHz)", data[self.TIME_STAMP], self.pid, self.AIC_FREQUENCY,
                           {'Value': data[self.AIC_FREQUENCY]}])
            result.append(["Imon", data[self.TIME_STAMP], self.pid, self.IMON, {'Value': data[self.IMON]}])
            result.append(["TEMP Warning 0", data[self.TIME_STAMP], self.pid, self.WARN_CNT0,
                           {'Value': data[self.WARN_CNT0]}])
            result.append(["TEMP Warning 1", data[self.TIME_STAMP], self.pid, self.WARN_CNT1,
                           {'Value': data[self.WARN_CNT1]}])
            result.append(["TEMP Warning 2", data[self.TIME_STAMP], self.pid, self.WARN_CNT2,
                           {'Value': data[self.WARN_CNT2]}])
            result.append(["TEMP Warning 3", data[self.TIME_STAMP], self.pid, self.WARN_CNT3,
                           {'Value': data[self.WARN_CNT3]}])
        _trace = TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, result)
        result = TraceViewManager.metadata_event(self.get_timeline_header())
        result.extend(_trace)
        return result
