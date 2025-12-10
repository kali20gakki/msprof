import os
import datetime

import numpy as np
import torch
import torch_npu

CALCULATE_DEVICE = "npu:3"
device = torch.device(CALCULATE_DEVICE)
torch.npu.set_device(device)

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
    schedule=torch_npu.profiler.schedule(wait=0, warmup=0, active=1, repeat=1, skip_first=0),
    on_trace_ready=torch_npu.profiler.tensorboard_trace_handler("./result"),
    record_shapes=False,
    profile_memory=False,
    with_stack=False,
    with_modules=False,
    with_flops=False,
    experimental_config=experimental_config
)


def run_add():
    # add
    a = torch.full((1000,), 1).npu()
    b = a
    for _ in range(1):
        b = b + 1


def run_mix_group_matmul():
    # mix aic group_matmul
    group_matmul_x1 = torch.randn(256, 256, device='npu', dtype=torch.float16)
    group_matmul_x2 = torch.randn(1024, 256, device='npu', dtype=torch.float16)
    group_matmul_x3 = torch.randn(512, 1024, device='npu', dtype=torch.float16)

    group_matmul_weight1 = torch.randn(256, 256, device='npu', dtype=torch.float16)
    group_matmul_weight2 = torch.randn(256, 1024, device='npu', dtype=torch.float16)
    group_matmul_weight3 = torch.randn(1024, 128, device='npu', dtype=torch.float16)

    group_matmul_bias1 = torch.randn(256, device='npu', dtype=torch.float16)
    group_matmul_bias2 = torch.randn(1024, device='npu', dtype=torch.float16)
    group_matmul_bias3 = torch.randn(128, device='npu', dtype=torch.float16)

    group_list = None
    split_item = 0
    x = [group_matmul_x1, group_matmul_x2, group_matmul_x3]
    weight = [group_matmul_weight1, group_matmul_weight2, group_matmul_weight3]
    bias = [group_matmul_bias1, group_matmul_bias2, group_matmul_bias3]
    for i in range(100):
        torch_npu.npu_grouped_matmul(x, weight, bias=bias, group_list=group_list, split_item=split_item)


def run_mix_moe_init_routing():
    # mix aiv moe_init_routing
    moe_init_routing_x = torch.tensor([[0.1, 0.1, 0.1, 0.1], [0.2, 0.2, 0.2, 0.2], [0.3, 0.3, 0.3, 0.3]],
                                      dtype=torch.float32).to("npu")
    moe_init_routing_row_idx = torch.tensor([[0, 3], [1, 4], [2, 5]], dtype=torch.int32).to("npu")
    moe_init_routing_expert_idx = torch.tensor([[1, 2], [0, 1], [0, 2]], dtype=torch.int32).to("npu")

    active_num = 3
    for i in range(10):
        torch_npu.npu_moe_init_routing(moe_init_routing_x, moe_init_routing_row_idx,
                                       moe_init_routing_expert_idx, active_num)


def run_matmul():
    # matmul
    aic_t1 = torch.rand((256, 1024), requires_grad=False).npu()
    aic_t2 = torch.rand((1024, 2048), requires_grad=False).npu()
    for i in range(20):
        torch.matmul(aic_t1, aic_t2)


def run():
    print(f"start")

    for _ in range(1):
        # run_matmul()
        run_add()
        # run_mix_group_matmul()
        # run_mix_moe_init_routing()

    print(f"end")


def main():

    prof.start()

    for i in range(1):
        run()
        prof.step()

    prof.stop()


if __name__ == '__main__':
    main()

