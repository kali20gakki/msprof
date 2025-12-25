#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

from abc import ABC

from common_func.constant import Constant
from common_func.data_manager import DataManager
from common_func.ms_constant.str_constant import StrConstant
from msmodel.l2_cache.soc_pmu_model import SocPmuViewerModel
from viewer.interface.base_viewer import BaseViewer


class SocPmuViewer(BaseViewer, ABC):
    """
    class for get soc pmu data
    """

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.param = params
        self.model_list = {
            "soc_pmu": SocPmuViewerModel
        }

    def get_summary_data(self: any) -> tuple:
        """
        to get summary data
        """
        summary_data = self.get_data_from_db()
        op_dict = DataManager.get_op_dict(self.params.get(StrConstant.PARAM_RESULT_DIR))
        headers = self.configs.get(StrConstant.CONFIG_HEADERS)
        if op_dict:
            headers.append("Op Name")
            for index, data in enumerate(summary_data):
                key = "{}-{}".format(data[1], data[0])  # 0 is stream id index, 1 is task id index
                data_list = list(data)
                data_list.append(op_dict.get(key, Constant.NA))
                summary_data[index] = data_list
        return headers, summary_data, len(summary_data)
