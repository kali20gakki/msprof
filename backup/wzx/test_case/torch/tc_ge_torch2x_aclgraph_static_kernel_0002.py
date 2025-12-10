#!/usr/bin/python3
# -*- coding: utf-8 -*-
import sys
import logging

import torch
import torch_npu
import torchair
from torchair import logger

# 设置Debug日志级别
logger.setLevel(logging.ERROR)

SKIP_FIRST = 0

# 图模式纯静态图，配置kernel_aot_optimization为true，profiling采集性能数据，验证走到静态kernel流程
def tc_ge_torch2x_aclgraph_static_kernel_0002(kernel_path, case_path, case_name, soc_version, device_id):
    print("Current case path: %s" % case_path)
    print("Current kernel compile path: %s" % kernel_path)
    print("Current soc version: %s" % soc_version)
    print("Current device id: %s" % device_id)
    print("Begin to execute test case %s" % case_name)
    dev_target = ['Ascend910B', 'Ascend910C']

    # build graph
    print("torchair场景构造模型，模型中包含relu、reshape、square算子，单输入单输出")
    class Network(torch.nn.Module):
        def __init__(self):
            super().__init__()
            self.relu = torch.nn.ReLU()

        def forward(self, data1):
            relu_01 = self.relu(data1)
            reshape_01 = torch.reshape(relu_01, (1, 32, 1, 128))
            softmax_01 = torch.nn.functional.softmax(reshape_01, dim=1)
            sqrt_01 = torch.sqrt(softmax_01)
            relu_02 = self.relu(sqrt_01)
            square_01 = torch.square(relu_02)
            add_01 = torch.add(square_01, square_01)
            return add_01
    print("step end")

    # input，torch场景构造一个输入
    print("torch场景构造一个输入")
    input0 = torch.randn(1, 4, 8, 128, dtype=torch.float16)
    print("step end")

    # run cpu
    print("torch2x,run cpu")
    cpu_mode = Network()
    cpu_mode = torch.compile(cpu_mode)
    cpu_out = cpu_mode(input0).detach().numpy()
    print("step end")

    # run npu
    print("torch2x,run npu，profling采集性能数据")
    print("run_torch2.X_subprocess with action type : [npu]")
    torch.npu.set_device(int(device_id))
    input0 = input0.npu()
    config = torchair.CompilerConfig()
    config.mode = "reduce-overhead"
    config.experimental_config.aclgraph._aclnn_static_shape_kernel = True
    npu_mode = Network().npu()
    npu_backend = torchair.get_npu_backend(compiler_config=config)
    npu_mode = torch.compile(npu_mode, fullgraph=True, backend=npu_backend, dynamic=False)

    # profiling采集性能数据
    experimental_config = torch_npu.profiler._ExperimentalConfig(
        profiler_level=torch_npu.profiler.ProfilerLevel.Level2,
        aic_metrics=torch_npu.profiler.AiCMetrics.PipeUtilization,
        data_simplification=False
    )

    with torch_npu.profiler.profile(
        activities=[
            torch_npu.profiler.ProfilerActivity.CPU,
            torch_npu.profiler.ProfilerActivity.NPU
        ],
        schedule=torch_npu.profiler.schedule(wait=0, warmup=0, active=2, repeat=1, skip_first=SKIP_FIRST),
        on_trace_ready=torch_npu.profiler.tensorboard_trace_handler("./result"),
        profile_memory=True,
        experimental_config=experimental_config) as prof:

        for i in range(3):
            npu_out = npu_mode(input0)
            prof.step()

    npu_out = npu_out.cpu().detach().numpy()
    print("step end")

    return



if __name__ == "__main__":
    device_id = 0
    if len(sys.argv) > 1:
        device_id = sys.argv[1]
    if len(sys.argv) > 2:
        status = sys.argv[2]
        if status == "REPLAY":
            SKIP_FIRST = 1
    tc_ge_torch2x_aclgraph_static_kernel_0002("./", "./", "test", "Ascend910B", device_id)

