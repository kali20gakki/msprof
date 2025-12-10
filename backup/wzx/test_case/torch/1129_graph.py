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
    acl_graph_model = AutoModelForCausalLM.from_pretrained(
        model_path,
        torch_dtype=torch.float16,
        trust_remote_code=True
    ).npu()

    ge_model = AutoModelForCausalLM.from_pretrained(
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
    for layer_id, transformer_block in enumerate(acl_graph_model.model.layers):
        attn_layer = transformer_block.self_attn
        attn_layer.hidden_size = attn_layer.hidden_size // tp_mesh.size()
        attn_layer.num_heads = attn_layer.num_heads // tp_mesh.size()
        attn_layer.num_key_value_heads = attn_layer.num_key_value_heads // tp_mesh.size()
        parallelize_module(
            module=transformer_block,
            device_mesh=tp_mesh,
            parallelize_plan=layer_tp_plan,
        )

    for layer_id, transformer_block in enumerate(ge_model.model.layers):
        attn_layer = transformer_block.self_attn
        attn_layer.hidden_size = attn_layer.hidden_size // tp_mesh.size()
        attn_layer.num_heads = attn_layer.num_heads // tp_mesh.size()
        attn_layer.num_key_value_heads = attn_layer.num_key_value_heads // tp_mesh.size()
        parallelize_module(
            module=transformer_block,
            device_mesh=tp_mesh,
            parallelize_plan=layer_tp_plan,
        )

    acl_graph_config = CompilerConfig()
    acl_graph_config.mode = "reduce-overhead"
    acl_graph_model = torch.compile(acl_graph_model, backend=torchair.get_npu_backend(compiler_config=acl_graph_config), dynamic=False)
    ge_config = CompilerConfig()
    ge_config.mode = "max-autotune"
    # ge_model = torch.compile(ge_model, backend=torchair.get_npu_backend(compiler_config=ge_config), dynamic=False)

    texts = ["What is the theory of relativity?"] * 10

    if dist.get_rank() == 0:
        print("开始进行推理")

    experimental_config = torch_npu.profiler._ExperimentalConfig(
        export_type=[
            torch_npu.profiler.ExportType.Text
        ],
        profiler_level=torch_npu.profiler.ProfilerLevel.Level0,
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
        schedule=torch_npu.profiler.schedule(wait=0, warmup=0, active=4, repeat=1, skip_first=2),
        on_trace_ready=torch_npu.profiler.tensorboard_trace_handler("./result"),
        record_shapes=False,
        profile_memory=False,
        with_stack=False,
        with_modules=False,
        with_flops=False,
        experimental_config=experimental_config)

    prof.start()

    for text in texts:
        acl_graph_inputs = tokenizer(text, return_tensors="pt").to(acl_graph_model.device)
        ge_inputs = tokenizer(text, return_tensors="pt").to(ge_model.device)

        # 生成文本
        with torch.no_grad():
            if dist.get_rank() == 0:
                start_time = time.time()
            acl_graph_model(**acl_graph_inputs)
            # ge_model(**ge_inputs)
            torch.npu.synchronize()
            if dist.get_rank() == 0:
                end_time = time.time()
                print(f"生成耗时: {end_time - start_time}秒")
            torch.npu.synchronize()
            prof.step()

    # 清理资源
    dist.destroy_process_group()

    prof.stop()


if __name__ == "__main__":
    main()
