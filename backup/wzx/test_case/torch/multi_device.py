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

experimental_config = torch_npu.profiler._ExperimentalConfig(
    export_type=[torch_npu.profiler.ExportType.Text, torch_npu.profiler.ExportType.Db],
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
    # schedule=torch_npu.profiler.schedule(wait=1, warmup=1, active=15, repeat=1, skip_first=1),
    on_trace_ready=torch_npu.profiler.tensorboard_trace_handler("./result"),
    record_shapes=False,
    profile_memory=False,
    with_stack=False,
    with_modules=False,
    with_flops=False,
    experimental_config=experimental_config
)


def run_matmul():
    aic_t1 = torch.rand((16384, 1024), requires_grad=False).npu()
    aic_t2 = torch.rand((1024, 16384), requires_grad=False).npu()
    for i in range(5):
        torch.matmul(aic_t1, aic_t2)


def run(rank_id: int):
    CALCULATE_DEVICE = "npu:" + str(rank_id)
    device = torch.device(CALCULATE_DEVICE)
    torch.npu.set_device(device)

    print(f"start")
    for _ in range(1):
        run_matmul()
    print(f"end")


def main():

    CALCULATE_DEVICE = "npu:2"
    device = torch.device(CALCULATE_DEVICE)
    torch.npu.set_device(device)

    prof.start()

    for i in range(30):
        run(i % 4 + 2)
        prof.step()

    prof.stop()


if __name__ == '__main__':
    main()

