#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import logging
from msconfig.meta_config import MetaConfig
from msconfig.prof_condition_config import ProfConditionConfig
from msconfig.prof_rule_config import ProfRuleConfig
from msconfig.data_parsers_config import DataParsersConfig
from msconfig.stars_config import StarsConfig
from msconfig.tables_config import TablesConfig
from msconfig.tables_training_config import TablesTrainingConfig
from msconfig.tables_operator_config import TablesOperatorConfig
from msconfig.msprof_export_data_config import MsProfExportDataConfig
from msconfig.data_calculator_config import DataCalculatorConfig
from msconfig.tuning_rule_config import TuningRuleConfig
from msconfig.ai_core_config import AICoreConfig
from msconfig.ai_cpu_config import AICPUConfig
from msconfig.ctrl_cpu_config import CtrlCPUConfig
from msconfig.ts_cpu_config import TsCPUConfig
from msconfig.constant_config import ConstantConfig
from msconfig.l2_cache_config import L2CacheConfig


class ConfigManager:
    PROF_CONDITION = 'ProfConditionConfig'
    PROF_RULE = 'ProfRuleConfig'
    DATA_PARSERS = 'DataParsersConfig'
    STARS = 'StarsConfig'
    TABLES = 'TablesConfig'
    TABLES_TRAINING = 'TablesTrainingConfig'
    TABLES_OPERATOR = 'TablesOperatorConfig'
    MSPROF_EXPORT_DATA = 'MsProfExportDataConfig'
    DATA_CALCULATOR = 'DataCalculatorConfig'
    TUNING_RULE = 'TuningRuleConfig'
    AI_CORE = "AICoreConfig"
    AI_CPU = "AICPUConfig"
    CTRL_CPU = "CtrlCPUConfig"
    TS_CPU = "TsCPUConfig"
    CONSTANT = "ConstantConfig"
    L2_CACHE = "L2CacheConfig"

    CONFIG_MAP = {
        PROF_CONDITION: ProfConditionConfig,
        PROF_RULE: ProfRuleConfig,
        DATA_PARSERS: DataParsersConfig,
        STARS: StarsConfig,
        TABLES: TablesConfig,
        TABLES_TRAINING: TablesTrainingConfig,
        TABLES_OPERATOR: TablesOperatorConfig,
        MSPROF_EXPORT_DATA: MsProfExportDataConfig,
        DATA_CALCULATOR: DataCalculatorConfig,
        TUNING_RULE: TuningRuleConfig,
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
