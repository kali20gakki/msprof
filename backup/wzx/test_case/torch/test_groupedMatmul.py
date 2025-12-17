# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------
import os
import datetime

import numpy as np
import torch
import torch_npu

CALCULATE_DEVICE = "npu:6"
device = torch.device(CALCULATE_DEVICE)
torch.npu.set_device(device)

experimental_config = torch_npu.profiler._ExperimentalConfig(
    export_type=[torch_npu.profiler.ExportType.Text],
    profiler_level=torch_npu.profiler.ProfilerLevel.Level1,
    msprof_tx=False,
    aic_metrics=torch_npu.profiler.AiCMetrics.AiCoreNone,
    l2_cache=False,
    op_attr=False,
    data_simplification=False
)
prof = torch_npu.profiler.profile(
    activities=[
        torch_npu.profiler.ProfilerActivity.CPU,
        torch_npu.profiler.ProfilerActivity.NPU
    ],
    on_trace_ready=torch_npu.profiler.tensorboard_trace_handler("./result"),
    record_shapes=False,
    profile_memory=False,
    with_stack=False,
    with_modules=False,
    with_flops=False,
    experimental_config=experimental_config
)

# mix group matmul
group_matmul_x1 = torch.randn(256, 256, device='npu', dtype=torch.float16)
group_matmul_x2 = torch.randn(1024, 256, device='npu', dtype=torch.float16)
group_matmul_x3 = torch.randn(512, 1024, device='npu', dtype=torch.float16)

group_matmul_weight1 = torch.randn(256, 256, device='npu', dtype=torch.float16)
group_matmul_weight2 = torch.randn(256, 1024, device='npu', dtype=torch.float16)
group_matmul_weight3 = torch.randn(1024, 128, device='npu', dtype=torch.float16)

group_matmul_bias1 = torch.randn(256, device='npu', dtype=torch.float16)
group_matmul_bias2 = torch.randn(1024, device='npu', dtype=torch.float16)
group_matmul_bias3 = torch.randn(128, device='npu', dtype=torch.float16)

def run_mix_group_matmul():
    # mix aic group_matmul
    group_list = None
    split_item = 0
    x = [group_matmul_x1, group_matmul_x2, group_matmul_x3]
    weight = [group_matmul_weight1, group_matmul_weight2, group_matmul_weight3]
    bias = [group_matmul_bias1, group_matmul_bias2, group_matmul_bias3]
    for i in range(100):
        torch_npu.npu_grouped_matmul(x, weight, bias=bias, group_list=group_list, split_item=split_item, group_type=-1)


def run():
    print("start")

    for _ in range(3):
        run_mix_group_matmul()

    print("end")

def main():
    run()
	

if __name__ == '__main__':
    main()
