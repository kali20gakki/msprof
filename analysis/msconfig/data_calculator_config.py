#!/usr/bin/python3
# -*- coding: utf-8 -*-
#  Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from msconfig.meta_config import MetaConfig


class DataCalculatorConfig(MetaConfig):
    DATA = {
        'AclCalculator': [
            ('path', 'mscalculate.acl.acl_calculator'),
            ('chip_model', '0,1,2,3,4,5')
        ]
        , 'SubTaskCalculator': [
            ('path', 'mscalculate.stars.sub_task_calculate'),
            ('chip_model', '5'),
            ('level', '2')
        ]
        , 'L2CacheCalculator': [
            ('path', 'mscalculate.l2_cache.l2_cache_calculator'),
            ('chip_model', '1,2,3,4')
        ]
        , 'HwtsCalculator': [
            ('path', 'mscalculate.hwts.hwts_calculator'),
            ('chip_model', '1,2,3,4')
        ]
        , 'HwtsAivCalculator': [
            ('path', 'mscalculate.hwts.hwts_aiv_calculator'),
            ('chip_model', '2,3,4')
        ]
        , 'AicCalculator': [
            ('path', 'mscalculate.aic.aic_calculator'),
            ('chip_model', '1,2,3,4')
        ]
        , 'MiniAicCalculator': [
            ('path', 'mscalculate.aic.mini_aic_calculator'),
            ('chip_model', '0')
        ]
        , 'AivCalculator': [
            ('path', 'mscalculate.aic.aiv_calculator'),
            ('chip_model', '2,3,4')
        ]
        , 'MemcpyCalculator': [
            ('path', 'mscalculate.memory_copy.memcpy_calculator'),
            ('chip_model', '0,1,2,3,4,5')
        ]
        , 'GeHashCalculator': [
            ('path', 'mscalculate.ge.ge_hash_calculator'),
            ('chip_model', '0,1,2,3,4,5,7')
        ]
        , 'StarsLogCalCulator': [
            ('path', 'msparser.stars.stars_log_parser'),
            ('chip_model', '5,7')
        ]
        , 'BiuPerfCalculator': [
            ('path', 'mscalculate.biu_perf.biu_perf_calculator'),
            ('chip_model', '5,7')
        ]
        , 'AcsqCalculator': [
            ('path', 'mscalculate.hwts.acsq_calculator'),
            ('chip_model', '5,7'),
            ('level', '2')
        ]
        , 'FftsPmuCalculate': [
            ('path', 'mscalculate.stars.ffts_pmu_calculate'),
            ('chip_model', '5,7'),
            ('level', '2')
        ]
        , 'AICpuFromTsCalculator': [
            ('path', 'mscalculate.ts_task.ai_cpu.aicpu_from_ts'),
            ('chip_model', '1')
        ]
    }
