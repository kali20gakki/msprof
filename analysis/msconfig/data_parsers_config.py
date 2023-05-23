#!/usr/bin/python3
# -*- coding: utf-8 -*-
#  Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.

from msconfig.meta_config import MetaConfig


class DataParsersConfig(MetaConfig):
    DATA = {
        'GeInfoParser': [
            ('path', 'msparser.ge.ge_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ],
        'GeModelInfoParser': [
            ('path', 'msparser.ge.ge_model_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ],
        'GeModelTimeParser': [
            ('path', 'msparser.ge.ge_model_time_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ],
        'GeHashParser': [
            ('path', 'msparser.ge.ge_hash_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ],
        'ParsingRuntimeData': [
            ('path', 'analyzer.create_runtime_db'),
            ('chip_model', '0,1,2,3,4,5,7')
        ],
        'AclParser': [
            ('path', 'msparser.acl.acl_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ],
        'L2CacheParser': [
            ('path', 'msparser.l2_cache.l2_cache_parser'),
            ('chip_model', '1,2,3,4,5,7')
        ],
        'TsTimelineRecParser': [
            ('path', 'msparser.iter_rec.ts_timeline_parser'),
            ('chip_model', '0'),
            ('level', '3')
        ],
        'ParsingDDRData': [
            ('path', 'msparser.hardware.ddr_parser'),
            ('chip_model', '0,1,2,3,4,7')
        ],
        'ParsingPeripheralData': [
            ('path', 'msparser.hardware.dvpp_parser'),
            ('chip_model', '0,1,5,7')
        ],
        'ParsingNicData': [
            ('path', 'msparser.hardware.nic_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ],
        'ParsingRoceData': [
            ('path', 'msparser.hardware.roce_parser'),
            ('chip_model', '0,1,2,3,4,5')
        ],
        'ParsingTSData': [
            ('path', 'msparser.hardware.tscpu_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ],
        'ParsingAICPUData': [
            ('path', 'msparser.hardware.ai_cpu_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ],
        'ParsingCtrlCPUData': [
            ('path', 'msparser.hardware.ctrl_cpu_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ],
        'ParsingMemoryData': [
            ('path', 'msparser.hardware.sys_mem_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ],
        'ParsingCpuUsageData': [
            ('path', 'msparser.hardware.sys_usage_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ],
        'ParsingPcieData': [
            ('path', 'msparser.hardware.pcie_parser'),
            ('chip_model', '0,1,2,3,4,5')
        ],
        'ParsingHBMData': [
            ('path', 'msparser.hardware.hbm_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ],
        'NonMiniLLCParser': [
            ('path', 'msparser.hardware.llc_parser'),
            ('chip_model', '1,2,3,4,5,7')
        ],
        'MiniLLCParser': [
            ('path', 'msparser.hardware.mini_llc_parser'),
            ('chip_model', '0')
        ],
        'ParsingHCCSData': [
            ('path', 'msparser.hardware.hccs_parser'),
            ('chip_model', '0,1,2,3,4,5')
        ],
        'TstrackParser': [
            ('path', 'msparser.step_trace.ts_track_parser'),
            ('chip_model', '1,2,3,4,5,7'),
            ('level', '2')
        ],
        'RtsTrackParser': [
            ('path', 'msparser.runtime.rts_parser'),
            ('chip_model', '0,1,2,3,4')
        ],
        'ParsingAICoreSampleData': [
            ('path', 'msparser.aic_sample.ai_core_sample_parser'),
            ('chip_model', '0,1,2,3,4')
        ],
        'ParsingAIVectorCoreSampleData': [
            ('path', 'msparser.aic_sample.ai_core_sample_parser'),
            ('chip_model', '2,3,4')
        ],
        'HCCLParser': [
            ('path', 'msparser.hccl.hccl_parser'),
            ('chip_model', '0,1,2,3,4,5'),
            ('level', '3')
        ],
        'MsprofTxParser': [
            ('path', 'msparser.msproftx.msproftx_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ],
        'RunTimeApiParser': [
            ('path', 'msparser.runtime.runtime_api_parser'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '2')
        ],
        'ParseAiCpuDataAdapter': [
            ('path', 'msparser.aicpu.parse_aicpu_data_adapter'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '3')
        ],
        'ParsingFftsAICoreSampleData': [
            ('path', 'msparser.aic_sample.ai_core_sample_parser'),
            ('chip_model', '5,7')
        ],
        'BiuPerfParser': [
            ('path', 'msparser.biu_perf.biu_perf_parser'),
            ('chip_model', '5,7')
        ],
        'SocProfilerParser': [
            ('path', 'msparser.stars.soc_profiler_parser'),
            ('chip_model', '5,7')
        ],
        'MsTimeParser': [
            ('path', 'msparser.ms_timer.ms_time_parser'),
            ('chip_model', '0,1,2,3,4,5')
        ],
        'DataPreparationParser': [
            ('path', 'msparser.aicpu.data_preparation_parser'),
            ('chip_model', '0,1,2,3,4,5')
        ],
        'HCCLOperatiorParser': [
            ('path', 'msparser.parallel.hccl_operator_parser'),
            ('chip_model', '1,2,3,4,5'),
            ('level', '3')
        ],
        'ParallelStrategyParser': [
            ('path', 'msparser.parallel.parallel_strategy_parser'),
            ('chip_model', '1,2,3,4,5')
        ],
        'ParallelParser': [
            ('path', 'msparser.parallel.parallel_parser'),
            ('chip_model', '1,2,3,4,5'),
            ('level', '4')
        ],
        'NpuMemParser': [
            ('path', 'msparser.npu_mem.npu_mem_parser'),
            ('chip_model', '0,1,3,4,5,7')
        ],
        'FreqParser': [
            ('path', 'msparser.freq.freq_parser'),
            ('chip_model', '5')
        ],
        'ApiDataParser': [
            ('path', 'msparser.api.api_data_parser'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '2')
        ],
        'EventDataParser': [
            ('path', 'msparser.event.event_data_parser'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '2')
        ],
        'HashDicParser': [
            ('path', 'msparser.hash_dic.hash_dic_parser'),
            ('chip_model', '0,1,2,3,4,5,7')
        ],
        'TaskTrackParser': [
            ('path', 'msparser.compact_info.task_track_parser'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '2'),
        ],
        'MemcpyInfoParser': [
            ('path', 'msparser.compact_info.memcpy_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '2'),
        ],
        'HcclInfoParser': [
            ('path', 'msparser.add_info.hccl_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '2')
        ],
        'MultiThreadParser': [
            ('path', 'msparser.add_info.multi_thread_parser'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '2')
        ],
        'TensorAddInfoParser': [
            ('path', 'msparser.add_info.tensor_add_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '2'),
        ],
        'FusionAddInfoParser': [
            ('path', 'msparser.add_info.fusion_add_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '2'),
        ],
        'GraphAddInfoParser': [
            ('path', 'msparser.add_info.graph_add_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '2'),
        ],
        'NodeBasicInfoParser': [
            ('path', 'msparser.compact_info.node_basic_info_parser'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '2'),
        ],
        'MemoryApplicationParser': [
            ('path', 'msparser.add_info.memory_application_parser'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '2'),
        ],
        'CtxIdParser': [
            ('path', 'msparser.add_info.ctx_id_parser'),
            ('chip_model', '5,7'),
            ('level', '2'),
        ],
        'CANNCalculator': [
            ('path', 'mscalculate.cann.cann_calculator'),
            ('chip_model', '0,1,2,3,4,5,7'),
            ('level', '3'),
            ('position', 'H')
        ],
        'IterRecParser': [
            ('path', 'msparser.iter_rec.iter_rec_parser'),
            ('chip_model', '1,2,3,4'),
            ('level', '4'),
            ('position', 'D')
        ],
        'NoGeIterRecParser': [
            ('path', 'msparser.iter_rec.iter_rec_parser'),
            ('chip_model', '1,2,3,4'),
            ('level', '4'),
            ('position', 'D')
        ],
        'StarsIterRecParser': [
            ('path', 'msparser.iter_rec.stars_iter_rec_parser'),
            ('chip_model', '5,7'),
            ('level', '4'),
            ('position', 'D')
        ],
    }
