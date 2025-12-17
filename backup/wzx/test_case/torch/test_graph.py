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
import time
import os
import torch
import torch_npu
import torch.distributed as dist
from transformers import AutoTokenizer, AutoModelForCausalLM
from torch.distributed.device_mesh import init_device_mesh
from torch.distributed.tensor.parallel import ColwiseParallel, RowwiseParallel, parallelize_module
import torchair
from torchair.configs.compiler_config import CompilerConfig


def main():
    model_path = "/home/msprof_smoke_test/model/ModelLink/model_from_hf/llama-2-7b-hf"

    # 初始化分布式环境
    def setup_distributed():
        rank = int(os.environ["RANK"])
        world_size = int(os.environ["WORLD_SIZE"])
        dist.init_process_group(backend='hccl', world_size=world_size, rank=rank)
        torch.npu.set_device(dist.get_rank())

    setup_distributed()

    # 使用正确的模型类
    tokenizer = AutoTokenizer.from_pretrained(model_path, trust_remote_code=True)
    model = AutoModelForCausalLM.from_pretrained(
        model_path,
        torch_dtype=torch.float16,
        trust_remote_code=True
    ).npu()

    # 创建张量并行设备网格
    tp_mesh = init_device_mesh("npu", (dist.get_world_size(),))

    # 定义正确的模块路径
    layer_tp_plan = {
        "self_attn.q_proj": ColwiseParallel(),
        "self_attn.k_proj": ColwiseParallel(),
        "self_attn.v_proj": ColwiseParallel(),
        "self_attn.o_proj": RowwiseParallel(),
        "mlp.gate_proj": ColwiseParallel(),
        "mlp.up_proj": ColwiseParallel(),
        "mlp.down_proj": RowwiseParallel()
    }

    if dist.get_rank() == 0:
        print("开始进行张量设置")

    # 对模型的每一层应用张量并行
    for layer_id, transformer_block in enumerate(model.model.layers):
        attn_layer = transformer_block.self_attn
        attn_layer.hidden_size = attn_layer.hidden_size // tp_mesh.size()
        attn_layer.num_heads = attn_layer.num_heads // tp_mesh.size()
        attn_layer.num_key_value_heads = attn_layer.num_key_value_heads // tp_mesh.size()
        parallelize_module(
            module=transformer_block,
            device_mesh=tp_mesh,
            parallelize_plan=layer_tp_plan,
        )

    config = CompilerConfig()
    config.mode = "reduce-overhead"
    # # config.experimental_config.frozen_parameter = True
    model = torch.compile(model, backend=torchair.get_npu_backend(compiler_config=config), dynamic=False)
    # model = torch.compile(model, backend='npugraphs', dynamic=False)
    # model = torch.compile(model, backend='inductor', mode='reduce-overhead', dynamic=False)

    texts = ["What is the theory of relativity?"] * 10

    if dist.get_rank() == 0:
        print("开始进行推理")

    experimental_config = torch_npu.profiler._ExperimentalConfig(
        export_type=[
            torch_npu.profiler.ExportType.Text
        ],
        profiler_level=torch_npu.profiler.ProfilerLevel.Level1,
        msprof_tx=False,
        aic_metrics=torch_npu.profiler.AiCMetrics.AiCoreNone,
        l2_cache=False,
        op_attr=False,
        data_simplification=False,
        record_op_args=False,
        gc_detect_threshold=None
    )

    prof = torch_npu.profiler.profile(
        activities=[
            torch_npu.profiler.ProfilerActivity.CPU,
            torch_npu.profiler.ProfilerActivity.NPU
        ],
        schedule=torch_npu.profiler.schedule(wait=0, warmup=0, active=4, repeat=1, skip_first=0),
        on_trace_ready=torch_npu.profiler.tensorboard_trace_handler("./result"),
        record_shapes=False,
        profile_memory=False,
        with_stack=False,
        with_modules=False,
        with_flops=False,
        experimental_config=experimental_config)

    prof.start()

    for text in texts:
        inputs = tokenizer(text, return_tensors="pt").to(model.device)

        # 生成文本
        with torch.no_grad():
            torch.npu.synchronize()
            if dist.get_rank() == 0:
                start_time = time.time()
            model(**inputs)
            torch.npu.synchronize()
            if dist.get_rank() == 0:
                end_time = time.time()
                print(f"生成耗时: {end_time - start_time}秒")
            prof.step()

            # # 解码输出
            # generated_text = tokenizer.decode(outputs[0], skip_special_tokens=True)

            # if dist.get_rank() == 0:
            #     print(f"生成文本: {generated_text}")

    # 清理资源
    dist.destroy_process_group()

    prof.stop()


if __name__ == "__main__":
    main()
