#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import logging
from config.meta_config import MetaConfig
from config.prof_condition_config import ProfConditionConfig
from config.prof_rule_config import ProfRuleConfig
from config.data_parsers_config import DataParsersConfig
from config.stars_config import StarsConfig
from config.tables_config import TablesConfig
from config.tables_training_config import TablesTrainingConfig
from config.tables_operator_config import TablesOperatorConfig
from config.msprof_export_data_config import MsProfExportDataConfig
from config.data_calculator_config import DataCalculatorConfig
from config.tuning_rule_config import TuningRuleConfig


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
        TUNING_RULE: TuningRuleConfig
    }

    @classmethod
    def get(cls: any, config_name: str):
        config_class = cls.CONFIG_MAP.get(config_name)
        if not config_class:
            config_class = MetaConfig
            logging.error('%s not found', config_name)
        return config_class()
