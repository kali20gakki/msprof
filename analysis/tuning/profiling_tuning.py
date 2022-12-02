#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.


from common_func.info_conf_reader import InfoConfReader
from tuning.meta_rule_manager import RuleManager


class ProfilingTuning:
    """
    recommend for inference
    """
    @classmethod
    def run(cls: any, result_dir: str) -> None:
        """
        run and recommend
        """
        devices = InfoConfReader().get_device_list()
        if devices and all(i.isdigit() for i in devices):
            for dev_id in devices:
                rule_mgr = RuleManager(result_dir, dev_id)
                rule_mgr.run()
