#!/usr/bin/python3
# -*- coding: utf-8 -*-

import os
import torch
import torch_npu
import torchair
import logging
from torchair import logger

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

def test_graph_capture_simple(data1):
    s = torch_npu.npu.Stream()
    experimental_config = torch_npu.profiler._ExperimentalConfig(
        aic_metrics=torch_npu.profiler.AiCMetrics.ArithmeticUtilization,
        profiler_level=torch_npu.profiler.ProfilerLevel.Level1,
        # record_op_args=True
    )
    prof = torch_npu.profiler.profile(
        activities=[
            torch_npu.profiler.ProfilerActivity.CPU,
            torch_npu.profiler.ProfilerActivity.NPU],
        schedule=torch_npu.profiler.schedule(wait=0, warmup=0, active=1, repeat=1, skip_first=0),
        on_trace_ready=torch_npu.profiler.tensorboard_trace_handler("./result"),
        experimental_config=experimental_config
    )

    prof.start()
    with torch_npu.npu.stream(s):
        # a = torch.full((1000,), 1, device="npu")
        g = torch_npu.npu.NPUGraph()
        torch_npu.npu.empty_cache()
        g.capture_begin()
        print("1111111111111111111111111111")
        npu_mode = Network().npu()
        npu_mode(data1)
        print("222222222222222222222222222222")
        g.capture_end()
    torch_npu.npu.current_stream().wait_stream(s)
    prof.stop()

    print("3333333333333333333333")
    prof.start()
    print("44444444444444444444444")
    g.replay()
    print("555555555555555555555555")
    prof.stop()
    print("66666666666666666666666")

input0 = torch.randn(1, 4, 8, 128, dtype=torch.float16)
input0 = input0.npu()

test_graph_capture_simple(input0)

