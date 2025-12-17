#!/usr/bin/python3
# -*- coding: utf-8 -*-
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
import torch
import torch_npu
import torchair
import logging
from torchair import logger
from torchair.configs.compiler_config import CompilerConfig

class Network(torch.nn.Module):

    def forward(self, a, b):
        y_npu = torch.matmul(a, b)
        return y_npu

def test_graph_capture_simple():
    a_shape = [4608, 2048]
    a = torch.randint(-10, 10, a_shape, dtype=torch.float32).npu()
    b_shape = [2048, 4096]
    b = torch.randint(-10, 10, b_shape, dtype=torch.float32).npu()
    

    config = CompilerConfig()
    config.mode = "reduce-overhead"
    config.experimental_config.aclgraph._aclnn_static_shape_kernel = True
    backend = torchair.get_npu_backend(compiler_config=config)

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
        g = torch_npu.npu.NPUGraph()
        torch_npu.npu.empty_cache()
        g.capture_begin()
        print("1111111111111111111111111111")
        npu_mode = Network().npu()
        output = npu_mode(a, b)
        #x_shape = [2048, 2048]
        #x = torch.randint(-10, 10, x_shape, dtype=torch.int32).npu()
        #activatition_scale = torch.randn((1, x_shape[1]), dtype=torch.float32).npu()
        #output0, output1 = npu_mode(x, weight_scale, activatition_scale, bias, quant_scale, quant_offset, group_index)
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


test_graph_capture_simple()

