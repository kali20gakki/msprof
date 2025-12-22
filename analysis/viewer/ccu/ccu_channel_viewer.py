#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

from abc import ABC

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import format_high_precision_for_csv
from msmodel.hardware.ccu_channel_model import CCUViewerChannelModel
from viewer.interface.base_viewer import BaseViewer


class CCUChannelViewer(BaseViewer, ABC):
    """
    class for get ccu channel data
    """
    PACKAGE_SIZE = 4 * Constant.KILOBYTE

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.model_list = {
            "ccu_channel": CCUViewerChannelModel
        }

    def get_summary_data(self: any) -> tuple:
        """
        to get summary data
        """
        summary_data = self.get_data_from_db()
        formatted_data = self.format_channel_summary_data(summary_data)
        return self.configs.get(StrConstant.CONFIG_HEADERS), formatted_data, len(formatted_data)

    def format_channel_summary_data(self, summary_data: list) -> list:
        return [(
            data.channel_id,
            format_high_precision_for_csv(InfoConfReader().trans_syscnt_into_local_time(data.timestamp)),
            round(self._trans_delay_to_bandwidth(data.min_bw), NumberConstant.ROUND_THREE_DECIMAL),
            round(self._trans_delay_to_bandwidth(data.max_bw), NumberConstant.ROUND_THREE_DECIMAL),
            round(self._trans_delay_to_bandwidth(data.avg_bw), NumberConstant.ROUND_THREE_DECIMAL)
        ) for data in summary_data
        ]

    def _trans_delay_to_bandwidth(self, syscnt: int) -> float:
        delay = InfoConfReader().duration_from_syscnt(syscnt)
        if delay == 0:
            return 0
        return self.PACKAGE_SIZE / delay * Constant.BYTE_US_TO_MB_S