#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from msconfig.meta_config import MetaConfig


class NanoAICoreConfig(MetaConfig):
    DATA = {
        'events': [

        ],
        'metrics': [
            ('cube_self_stall_cycles', ''),
            ('vec_self_stall_cycles', ''),
            ('cube_mte2_cflt_stall_cycles', ''),
            ('cube_mte3_cflt_stall_cycles', ''),
            ('cube_mte1_cflt_stall_cycles', ''),
            ('vec_mte2_cflt_stall_cycles', ''),
            ('vec_mte3_cflt_stall_cycles', ''),
            ('vec_mte1_cflt_stall_cycles', ''),
            ('mte1_mte2_stall_cycles', ''),
            ('mte1_mte3_stall_cycles', ''),
            ('mte2_mte3_stall_cycles', ''),
            ('su_stall_cycles', ''),
            ('scalar_ratio', ''),
            ('mte1_ratio', ''),
            ('mte2_ratio', ''),
            ('mte3_ratio', ''),
            ('mte_preload_ratio', ''),
            ('mac_exe_ratio', ''),
            ('vec_exe_ratio', ''),
            ('fixpipe_ratio', ''),
            ('acess_stack_ratio', ''),
            ('control_flow_prediction_ratio', ''),
            ('control_flow_mis_prediction_rate', ''),
            ('icache_miss_rate', ''),
            ('main_mem_read_bw(GB/s)', ''),
            ('main_mem_write_bw(GB/s)', ''),
            ('ub_read_bw_mte(GB/s)', ''),
            ('ub_write_bw_mte(GB/s)', ''),
            ('ub_read_bw_mte2(GB/s)', ''),
            ('ub_write_bw_mte2(GB/s)', ''),
            ('ub_read_bw_vector(GB/s)', ''),
            ('ub_write_bw_vector(GB/s)', ''),
            ('ub_read_bw_scalar(GB/s)', ''),
            ('ub_write_bw_scalar(GB/s)', ''),
        ],
        'formula': [

        ],
        'event2metric': [
            ('0x100', 'scalar_ratio'),
            ('0x101', 'icache_req_ratio'),
            ('0x102', 'icache_miss_rate'),
            ('0x103', 'acess_stack_ratio'),
            ('0x104', 'control_flow_prediction_ratio'),
            ('0x105', 'control_flow_mis_prediction_rate'),
            ('0x106', 'ub_read_bw_scalar(GB/s)'),
            ('0x107', 'ub_write_bw_scalar(GB/s)'),
            ('0x200', 'mte1_ratio'),
            ('0x201', 'mte2_ratio'),
            ('0x202', 'mte3_ratio'),
            ('0x203', 'mte_preload_ratio'),
            ('0x204', 'main_mem_read_bw(GB/s)'),
            ('0x205', 'main_mem_write_bw(GB/s)'),
            ('0x206', 'ub_read_bw_mte2(GB/s)'),
            ('0x207', 'ub_write_bw_mte2(GB/s)'),
            ('0x208', 'ub_read_bw_mte(GB/s)'),
            ('0x209', 'ub_write_bw_mte(GB/s)'),
            ('0x300', 'vec_exe_ratio'),
            ('0x302', 'fixpipe_ratio'),
            ('0x303', 'ub_read_bw_vector(GB/s)'),
            ('0x304', 'ub_write_bw_vector(GB/s)'),
            ('0x305', 'vec_self_stall_cycles'),
            ('0x400', 'mac_exe_ratio'),
            ('0x406', 'cube_self_stall_cycles'),
            ('0x600', 'cube_mte2_cflt_stall_cycles'),
            ('0x601', 'cube_mte3_cflt_stall_cycles'),
            ('0x602', 'cube_mte1_cflt_stall_cycles'),
            ('0x603', 'vec_mte2_cflt_stall_cycles'),
            ('0x604', 'vec_mte3_cflt_stall_cycles'),
            ('0x605', 'vec_mte1_cflt_stall_cycles'),
            ('0x606', 'mte1_mte2_stall_cycles'),
            ('0x607', 'mte1_mte3_stall_cycles'),
            ('0x608', 'mte2_mte3_stall_cycles'),
            ('0x609', 'su_stall_cycles'),
        ],
        'custom': [

        ]
    }

    def __init__(self):
        super().__init__()


