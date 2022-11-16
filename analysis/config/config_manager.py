#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


import logging
from config.meta_config import MetaConfig
from config.ai_core_config import AICoreConfig
from config.ai_cpu_config import AICPUConfig
from config.ctrl_cpu_config import CtrlCPUConfig
from config.ts_cpu_config import TsCPUConfig
from config.constant_config import ConstantConfig
from config.l2_cache_config import L2CacheConfig


class ConfigManager:
    AI_CORE = "AICoreConfig"
    AI_CPU = "AICPUConfig"
    CTRL_CPU = "CtrlCPUConfig"
    TS_CPU = "TsCPUConfig"
    CONSTANT = "ConstantConfig"
    L2_CACHE = "L2CacheConfig"
    CONFIG_MAP = {
        AI_CORE: AICoreConfig,
        AI_CPU: AICPUConfig,
        CTRL_CPU: CtrlCPUConfig,
        TS_CPU: TsCPUConfig,
        CONSTANT: ConstantConfig,
        L2_CACHE: L2CacheConfig
    }

    @classmethod
    def get(cls: any, config_name: str):
        config_class = cls.CONFIG_MAP.get(config_name)
        if not config_class:
            config_class = MetaConfig
            logging.error('%s not found', config_name)
        return config_class()

