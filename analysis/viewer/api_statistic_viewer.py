#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import logging

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from msmodel.api.api_data_viewer_model import ApiDataViewModel


class ApiStatisticViewer:
    """
    Viewer for showing API(ACL/Runtime API/GE) data
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        self._configs = configs
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._api_model = ApiDataViewModel(params)
        self.result = {}

    def api_statistic_reformat(self, data_list: list) -> list:
        reformat_result = []
        for data in data_list:
            try:
                n = len(self.result[data[0]])
                mean = sum(self.result[data[0]]) / n
                deviations = [(x - mean) ** 2 for x in self.result[data[0]]]
                variance = sum(deviations) / n
            except KeyError:
                logging.error("api data maybe parse failed, please check the data parsing log.")
            reformat_result.append(
                (
                    data[7],
                    data[0],
                    InfoConfReader().get_host_duration(data[2], NumberConstant.MICRO_SECOND),
                    data[3],
                    InfoConfReader().get_host_duration(data[4], NumberConstant.MICRO_SECOND),
                    InfoConfReader().get_host_duration(data[5], NumberConstant.MICRO_SECOND),
                    InfoConfReader().get_host_duration(data[6], NumberConstant.MICRO_SECOND),
                    variance
                )
            )
        return reformat_result

    def get_api_summary_data(self: any) -> list:
        """
        get summary data from api_event.db
        :return: summary data
        """
        if not self._api_model.init():
            logging.error("api data maybe parse failed, please check the data parsing log.")
            return MsvpConstant.EMPTY_LIST
        api_statistic_data = self._api_model.get_api_statistic_data()
        var_data = self._api_model.get_api_statistic_data_for_variance()
        for key, value in var_data:
            reformatted_value = InfoConfReader().get_host_duration(value, NumberConstant.MICRO_SECOND)
            self.result.setdefault(key, []).append(reformatted_value)
        api_statistic_data = self.api_statistic_reformat(api_statistic_data)
        if api_statistic_data:
            return api_statistic_data
        return MsvpConstant.EMPTY_LIST

    def get_api_statistic_data(self) -> tuple:
        """
         get api statistic data
        """
        data = self.get_api_summary_data()
        return self._configs.get(StrConstant.CONFIG_HEADERS), data, len(data)
