#!/usr/bin/python3
# coding=utf-8
"""
function: parser for ge task
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
from collections import defaultdict

from msmodel.stars.stars_chip_trans_model import StarsChipTransModel
from msparser.interface.istars_parser import IStarsParser
from profiling_bean.stars.stars_chip_trans_bean import StarsChipTransBean


class StarsChipTransParser(IStarsParser):
    """
    stars chip trans data parser
    """

    def __init__(self: any, result_dir: str, db: str, table_list: list) -> None:
        super().__init__()
        self._model = StarsChipTransModel(result_dir, db, table_list)
        self._decoder = StarsChipTransBean
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
        data_dict = {}
        for bean_data in self._data_list:
            data_dict.setdefault(bean_data.acc_type, []) \
                .append([bean_data.event_id, bean_data.pa_rx_or_pcie_write_bw,
                         bean_data.pa_tx_or_pcie_read_bw, bean_data.sys_time])
        self._data_list = data_dict
