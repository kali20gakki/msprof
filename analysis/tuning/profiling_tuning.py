#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.


from common_func.ms_constant.str_constant import StrConstant
from common_func.info_conf_reader import InfoConfReader
from tuning.meta_rule_manager import RuleManager


class ProfilingTuning:
    """
    recommend for inference
    """
    @classmethod
    def run(cls: any, result_dir: str, sample_config: dict) -> None:
        """
        run and recommend
        """
        devices = InfoConfReader().get_device_list()
        if devices and all(i.isdigit() for i in devices):
            for dev_id in devices:
                para = {
                    StrConstant.PARAM_RESULT_DIR: result_dir,
                    StrConstant.PARAM_DEVICE_ID: dev_id,
                    StrConstant.SAMPLE_CONFIG: sample_config
                }
                rule_mgr = RuleManager(para)
                rule_mgr.run()
