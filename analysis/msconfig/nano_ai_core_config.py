#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from msconfig.meta_config import MetaConfig


class NanoAICoreConfig(MetaConfig):
    DATA = {
        'events': [

        ],
        'metrics': [
            ('scalar_ratio', '1.0*r100/task_cyc'),
            ('mte1_ratio', '1.0*r200/task_cyc'),
            ('mte2_ratio', '1.0*r201/task_cyc'),
            ('mte3_ratio', '1.0*r202/task_cyc'),
            ('mte3_preload_ratio', '1.0*r203/task_cyc'),
            ('mac_exe_ratio', '1.0*r400/task_cyc'),
            ('vec_exe_ratio', '1.0*r300/task_cyc'),
            ('fixpipe_ratio', '1.0*r302/task_cyc'),
            ('icache_miss_rate', '1.0*r102/r101'),
            ('main_mem_read_bw(GB/s)',
             '1.0*r516*256.0*8.0/((task_cyc*1000/(freq*1000.0))/'
             'block_num*((block_num+core_num-1)/core_num))/(8589934592.0)'),
            ('main_mem_write_bw(GB/s)',
             '1.0*r517*256.0*8.0/((task_cyc*1000/(freq*1000.0))/'
             'block_num*((block_num+core_num-1)/core_num))/(8589934592.0)'),
            ('ub_read_bw_mte(GB/s)',
             '((r3d-r10*4.0-r13*4.0)*128.0 +(r10*4.0+r13*4.0)*64.0)/'
             '((task_cyc*1000/(freq*1000.0))/block_num*((block_num+core_num-1)/core_num))/(8589934592.0)'),
            ('ub_write_bw_mte(GB/s)',
             '(r211*128.0*8.0)/((task_cyc*1000/(freq*1000.0))/'
             'block_num*((block_num+core_num-1)/core_num))/(8589934592.0)'),
            ('ub_read_bw_vector(GB/s)',
             '1.0*r208*256.0*16.0/((task_cyc*1000/(freq*1000.0))/'
             'block_num*((block_num+core_num-1)/core_num))/(8589934592.0)'),
            ('ub_write_bw_vector(GB/s)',
             '1.0*r209*256.0*8.0/((task_cyc*1000/(freq*1000.0))/'
             'block_num*((block_num+core_num-1)/core_num))/(8589934592.0)'),
            ('ub_read_bw_scalar(GB/s)',
             '1.0*r204*256.0*32.0/((task_cyc*1000/(freq*1000.0))/'
             'block_num*((block_num+core_num-1)/core_num))/(8589934592.0)'),
            ('ub_write_bw_scalar(GB/s)',
             '1.0*r205*256.0*32.0/((task_cyc*1000/(freq*1000.0))/'
             'block_num*((block_num+core_num-1)/core_num))/(8589934592.0)'),
        ],
        'formula': [

        ],
        'event2metric': [
            ('0x100', 'scalar_ratio'),
            ('0x101', 'icache_req_ratio'),
            ('0x102', 'icache_miss_rate'),
            ('0x200', 'mte1_ratio'),
            ('0x201', 'mte2_ratio'),
            ('0x202', 'mte3_ratio'),
            ('0x203', 'mte3_preload_ratio'),
            ('0x204', 'ub_read_bw_scalar'),
            ('0x205', 'ub_write_bw_scalar'),
            ('0x207', 'cube_ub_wr_req_num'),
            ('0x208', 'ub_read_bw_vector'),
            ('0x209', 'ub_write_bw_vector'),
            ('0x210', 'ub_read_bw_mte'),
            ('0x211', 'ub_write_bw_mte'),
            ('0x300', 'vec_exe_ratio'),
            ('0x302', 'fixpipe_ratio'),
            ('0x400', 'mac_exe_ratio'),
            ('0x1026', 'cube_fm_ub_re_req_num'),
            ('0x1027', 'cube_wt_ub_re_req_num'),
            ('0x1028', 'cube_bias_ub_re_req_num')
        ],
        'custom': [

        ]
    }

    def __init__(self):
        super().__init__()


