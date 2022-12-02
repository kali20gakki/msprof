#!/usr/bin/python3
# -*- coding: utf-8 -*-
#  Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from config.meta_config import MetaConfig


class DataParsersConfig(MetaConfig):
    DATA = {
        'GeInfoParser': [
            ('path', 'msparser.ge.ge_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ]
        , 'GeModelInfoParser': [
            ('path', 'msparser.ge.ge_model_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ]
        , 'GeModelTimeParser': [
            ('path', 'msparser.ge.ge_model_time_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ]
        , 'GeHashParser': [
            ('path', 'msparser.ge.ge_hash_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ]
        , 'ParsingRuntimeData': [
            ('path', 'analyzer.create_runtime_db'),
            ('chip_model', '0,1,2,3,4,5,7')
        ]
        , 'AclParser': [
            ('path', 'msparser.acl.acl_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ]
        , 'L2CacheParser': [
            ('path', 'msparser.l2_cache.l2_cache_parser'),
            ('chip_model', '1,2,3,4')
        ]
        , 'IterRecParser': [
            ('path', 'msparser.iter_rec.iter_rec_parser'),
            ('chip_model', '1,2,3,4'),
            ('level', '3')
        ]
        , 'NoGeIterRecParser': [
            ('path', 'msparser.iter_rec.iter_rec_parser'),
            ('chip_model', '1,2,3,4'),
            ('level', '3')
        ]
        , 'TsTimelineRecParser': [
            ('path', 'msparser.iter_rec.ts_timeline_parser'),
            ('chip_model', '0'),
            ('level', '3')
        ]
        , 'ParsingDDRData': [
            ('path', 'msparser.hardware.ddr_parser'),
            ('chip_model', '0,1,2,3,4,7')
        ]
        , 'ParsingPeripheralData': [
            ('path', 'msparser.hardware.dvpp_parser'),
            ('chip_model', '0,1')
        ]
        , 'ParsingNicData': [
            ('path', 'msparser.hardware.nic_parser'),
            ('chip_model', '0,1,2,3,4,5')
        ]
        , 'ParsingRoceData': [
            ('path', 'msparser.hardware.roce_parser'),
            ('chip_model', '0,1,2,3,4,5')
        ]
        , 'ParsingTSData': [
            ('path', 'msparser.hardware.tscpu_parser'),
            ('chip_model', '0,1,2,3,4,5')
        ]
        , 'ParsingAICPUData': [
            ('path', 'msparser.hardware.ai_cpu_parser'),
            ('chip_model', '0,1,2,3,4,5')
        ]
        , 'ParsingCtrlCPUData': [
            ('path', 'msparser.hardware.ctrl_cpu_parser'),
            ('chip_model', '0,1,2,3,4,5')
        ]
        , 'ParsingMemoryData': [
            ('path', 'msparser.hardware.sys_mem_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ]
        , 'ParsingCpuUsageData': [
            ('path', 'msparser.hardware.sys_usage_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ]
        , 'ParsingPcieData': [
            ('path', 'msparser.hardware.pcie_parser'),
            ('chip_model', '0,1,2,3,4,5')
        ]
        , 'ParsingHBMData': [
            ('path', 'msparser.hardware.hbm_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ]
        , 'NonMiniLLCParser': [
            ('path', 'msparser.hardware.llc_parser'),
            ('chip_model', '1,2,3,4,5,7')
        ]
        , 'MiniLLCParser': [
            ('path', 'msparser.hardware.mini_llc_parser'),
            ('chip_model', '0')
        ]
        , 'ParsingHCCSData': [
            ('path', 'msparser.hardware.hccs_parser'),
            ('chip_model', '0,1,2,3,4,5')
        ]
        , 'TstrackParser': [
            ('path', 'msparser.step_trace.ts_track_parser'),
            ('chip_model', '1,2,3,4,5,7'),
            ('level', '2')
        ]
        , 'RtsTrackParser': [
            ('path', 'msparser.runtime.rts_parser'),
            ('chip_model', '0,1,2,3,4')
        ]
        , 'ParsingAICoreSampleData': [
            ('path', 'msparser.aic_sample.ai_core_sample_parser'),
            ('chip_model', '0,1,2,3,4')
        ]
        , 'ParsingAIVectorCoreSampleData': [
            ('path', 'msparser.aic_sample.ai_core_sample_parser'),
            ('chip_model', '2,3,4')
        ]
        , 'HCCLParser': [
            ('path', 'msparser.hccl.hccl_parser'),
            ('chip_model', '0,1,2,3,4'),
            ('level', '3')
        ]
        , 'MsprofTxParser': [
            ('path', 'msparser.msproftx.msproftx_parser'),
            ('chip_model', '0,1,2,3,4,5')
        ]
        , 'RunTimeApiParser': [
            ('path', 'msparser.runtime.runtime_api_parser'),
            ('chip_model', '0,1,2,3,4,5'),
            ('level', '2')
        ]
        , 'ParseAiCpuDataAdapter': [
            ('path', 'msparser.aicpu.parse_aicpu_data_adapter'),
            ('chip_model', '0,1,2,3,4,5'),
            ('level', '3')
        ]
        , 'GeHostParser': [
            ('path', 'msparser.ge.ge_host_parser'),
            ('chip_model', '0,1,2,3,4,5')
        ]
        , 'StarsIterRecParser': [
            ('path', 'msparser.iter_rec.stars_iter_rec_parser'),
            ('chip_model', '5,7'),
            ('level', '3')
        ]
        , 'ParsingFftsAICoreSampleData': [
            ('path', 'msparser.aic_sample.ai_core_sample_parser'),
            ('chip_model', '5,7')
        ]
        , 'BiuPerfParser': [
            ('path', 'msparser.biu_perf.biu_perf_parser'),
            ('chip_model', '5,7')
        ]
        , 'SocProfilerParser': [
            ('path', 'msparser.stars.soc_profiler_parser'),
            ('chip_model', '5,7')
        ]
        , 'MsTimeParser': [
            ('path', 'msparser.ms_timer.ms_time_parser'),
            ('chip_model', '0,1,2,3,4,5')
        ]
        , 'DataPreparationParser': [
            ('path', 'msparser.aicpu.data_preparation_parser'),
            ('chip_model', '0,1,2,3,4,5')
        ]
        , 'HCCLOperatiorParser': [
            ('path', 'msparser.parallel.hccl_operator_parser'),
            ('chip_model', '1,2,3,4,5'),
            ('level', '3')
        ]
        , 'ParallelStrategyParser': [
            ('path', 'msparser.parallel.parallel_strategy_parser'),
            ('chip_model', '1,2,3,4,5')
        ]
        , 'ParallelParser': [
            ('path', 'msparser.parallel.parallel_parser'),
            ('chip_model', '1,2,3,4,5'),
            ('level', '4')
        ]
        , 'L2CacheSampleParser': [
            ('path', 'msparser.l2_cache.l2_cache_sample_parser'),
            ('chip_model', '5')
        ]
    }
