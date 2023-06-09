#!/usr/bin/python3
# -*- coding: utf-8 -*-
#  Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from msconfig.meta_config import MetaConfig


class DataCalculatorConfig(MetaConfig):
    DATA = {
        'AclCalculator': [
            ('path', 'mscalculate.acl.acl_calculator'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('position', 'H')
        ],
        'SubTaskCalculator': [
            ('path', 'mscalculate.stars.sub_task_calculate'),
            ('chip_model', '5'),
            ('level', '4'),
            ('position', 'D')
        ],
        'L2CacheCalculator': [
            ('path', 'mscalculate.l2_cache.l2_cache_calculator'),
            ('chip_model', '1,2,3,4,5,7'),
            ('position', 'D')
        ],
        'HwtsCalculator': [
            ('path', 'mscalculate.hwts.hwts_calculator'),
            ('chip_model', '1,2,3,4'),
            ('level', '3'),
            ('position', 'D')
        ],
        'HwtsAivCalculator': [
            ('path', 'mscalculate.hwts.hwts_aiv_calculator'),
            ('chip_model', '2,3,4'),
            ('level', '3'),
            ('position', 'D')
        ],
        'AicCalculator': [
            ('path', 'mscalculate.aic.aic_calculator'),
            ('chip_model', '1,2,3,4'),
            ('level', '3'),
            ('position', 'D')
        ],
        'MiniAicCalculator': [
            ('path', 'mscalculate.aic.mini_aic_calculator'),
            ('chip_model', '0'),
            ('position', 'D')
        ],
        'AivCalculator': [
            ('path', 'mscalculate.aic.aiv_calculator'),
            ('chip_model', '2,3,4'),
            ('level', '3'),
            ('position', 'D')
        ],
        'MemcpyCalculator': [
            ('path', 'mscalculate.memory_copy.memcpy_calculator'),
            ('chip_model', '0,1,2,3,4,5'),
            ('position', 'D')
        ],
        'GeHashCalculator': [
            ('path', 'mscalculate.ge.ge_hash_calculator'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('position', 'H')
        ],
        'StarsLogCalCulator': [
            ('path', 'msparser.stars.stars_log_parser'),
            ('chip_model', '5,7'),
            ('level', '3'),
            ('position', 'D')
        ],
        'BiuPerfCalculator': [
            ('path', 'mscalculate.biu_perf.biu_perf_calculator'),
            ('chip_model', '5,7'),
            ('position', 'D')
        ],
        'AcsqCalculator': [
            ('path', 'mscalculate.hwts.acsq_calculator'),
            ('chip_model', '5,7'),
            ('level', '4'),
            ('position', 'D')
        ],
        'FftsPmuCalculate': [
            ('path', 'mscalculate.stars.ffts_pmu_calculate'),
            ('chip_model', '5,7'),
            ('level', '3'),
            ('position', 'D')
        ],
        'AICpuFromTsCalculator': [
            ('path', 'mscalculate.ts_task.ai_cpu.aicpu_from_ts'),
            ('chip_model', '1'),
            ('position', 'D')
        ],
        'TaskSchedulerCalculator': [
            ('path', 'mscalculate.data_analysis.task_scheduler_calculator'),
            ('chip_model', '0'),
            ('level', '13'),
            ('position', 'D')
        ],
        'OpTaskSchedulerCalculator': [
            ('path', 'mscalculate.data_analysis.op_task_scheduler_calculator'),
            ('chip_model', '0'),
            ('level', '13'),
            ('position', 'D')
        ],
        'ParseAiCoreOpSummaryCalculator': [
            ('path', 'mscalculate.data_analysis.parse_aicore_op_summary_calculator'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '14'),
            ('position', 'D')
        ],
        'OpSummaryOpSceneCalculator': [
            ('path', 'mscalculate.data_analysis.op_summary_op_scene_calculator'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '14'),
            ('position', 'D')
        ],
        'MergeOpCounterCalculator': [
            ('path', 'mscalculate.data_analysis.merge_op_counter_calculator'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '14'),
            ('position', 'D')
        ],
        'OpCounterOpSceneCalculator': [
            ('path', 'mscalculate.data_analysis.op_counter_op_scene_calculator'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '14'),
            ('position', 'D')
        ],
        'TorchAclRelationCalculator': [
            ('path', 'mscalculate.sync_acl_npu.torch_acl_relation'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '2'),
            ('position', 'H')
        ],
        'TorchNpuRelationCalculator': [
            ('path', 'mscalculate.sync_acl_npu.torch_npu_relation'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '3'),
            ('position', 'H')
        ],
        'HcclCalculator': [
            ('path', 'mscalculate.hccl_calculator'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '5'),
            ('position', 'D')
        ],
        'NpuOpMemCalculator': [
            ('path', 'mscalculate.npu_mem.npu_op_mem_calculator'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('position', 'D')
        ],
    }
