#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

import logging

from common_func.constant import Constant
from msmodel.stars.lowpower_model import LowPowerModel
from msparser.interface.istars_parser import IStarsParser
from profiling_bean.stars.lowpower_bean import LowPowerBean


class LowPowerParser(IStarsParser):
    """
    stars chip trans data parser
    """
    CURRENT_INDEX = 11
    POWER_INDEX = 12
    DATA_LENGTH = 37

    def __init__(self: any, result_dir: str, db: str, table_list: list) -> None:
        super().__init__()
        self._model = LowPowerModel(result_dir, db, table_list)
        self._decoder = LowPowerBean
        self._data_list = []

    @property
    def decoder(self: any) -> any:
        """
        get decoder
        :return: class decoder
        """
        return self._decoder

    def preprocess_data(self: any) -> None:
        """
        process data list before save to db
        :return: None
        """
        data_dict = []
        tmp_list = []
        for data in self._data_list:
            if not tmp_list:
                tmp_list.extend([data.sys_time] + list(data.lp_info))
                continue
            elif data.sys_time != tmp_list[0]:
                logging.warning('Can not match the data which time is %s', str(tmp_list[0]))
                tmp_list.clear()
                tmp_list.extend([data.sys_time] + list(data.lp_info))
            else:
                tmp_list.extend(list(data.lp_info))
            if len(tmp_list) < self.DATA_LENGTH:
                continue

            voltage = tmp_list[self.POWER_INDEX]
            try:
                voltage = tmp_list[self.POWER_INDEX] // tmp_list[self.CURRENT_INDEX]
            except ZeroDivisionError as er:
                logging.error(str(er), exc_info=Constant.TRACE_BACK_SWITCH)
            data_dict.append(tmp_list[0:29] + [voltage])
            tmp_list = []

        self._data_list = data_dict
