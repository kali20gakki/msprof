import os
import datetime

import numpy as np
import torch
import torch_npu
import torch.distributed as dist
from torch.distributed.distributed_c10d import _get_default_group
import torchair
from torchair.configs.compiler_config import CompilerConfig

LOCAL_RANK = 7
# LOCAL_RANK = int(os.environ['LOCAL_RANK'])
# RANK = int(os.environ['RANK'])
# WORLD_SIZE = int(os.environ['WORLD_SIZE'])

torch.npu.set_device(LOCAL_RANK)
# dist.init_process_group(backend='hccl', rank=RANK,
#                         world_size=WORLD_SIZE,
#                         timeout=datetime.timedelta(seconds=1800))

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
    # on_trace_ready=torch_npu.profiler.tensorboard_trace_handler("/home/result_dir/test_torch_capture_without_schedule_by_object_profiler/PytorchCaptureWithBeginEnd"),
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

# moe init routing
moe_init_routing_x = torch.tensor([[0.1, 0.1, 0.1, 0.1], [0.2, 0.2, 0.2, 0.2], [0.3, 0.3, 0.3, 0.3]],
                                  dtype=torch.float32).to("npu")
moe_init_routing_row_idx = torch.tensor([[0, 3], [1, 4], [2, 5]], dtype=torch.int32).to("npu")
moe_init_routing_expert_idx = torch.tensor([[1, 2], [0, 1], [0, 2]], dtype=torch.int32).to("npu")

# mc2
# mc2_input = torch.tensor(np.random.uniform(-5, 5, size=[122940, 1536])).to(torch.bfloat16).npu()
# mc2_weight = torch.tensor(np.random.uniform(-20, 20, size=[1536, 12288])).to(torch.bfloat16).npu()
# mc2_default_pg = _get_default_group()
# mc2_hcom_info = mc2_default_pg._get_backend(torch.device("npu")).get_hccl_comm_name(LOCAL_RANK)

# add
a = torch.full((1000,), 1).npu(LOCAL_RANK)

# matmul
aic_t1 = torch.rand((256, 1024), requires_grad=False).npu(LOCAL_RANK)
aic_t2 = torch.rand((1024, 2048), requires_grad=False).npu(LOCAL_RANK)

# all_gather
# input_tensor = torch.rand((1024, 1024), requires_grad=True).npu(LOCAL_RANK)
# output_list = [torch.empty((1024, 1024), requires_grad=True).npu(LOCAL_RANK) for _ in range(WORLD_SIZE)]

# def run_mc2():
#     # mc2
#     torch_npu.npu_mm_all_reduce_base(mc2_input, mc2_weight, mc2_hcom_info, reduce_op='sum')
#
#
# def run_all_gather():
#     # allGather
#     for _ in range(5):
#         dist.all_gather(output_list, input_tensor)
#
#
# def run_all_reduce():
#     # allReduce
#     out = torch.matmul(aic_t1, aic_t2)
#     for _ in range(5):
#         dist.all_reduce(out)


def run_add():
    # add
    b = a
    for _ in range(1000000):
        b = b + 1


# def run_mix_group_matmul():
#     # mix aic group_matmul
#     group_list = None
#     split_item = 0
#     x = [group_matmul_x1, group_matmul_x2, group_matmul_x3]
#     weight = [group_matmul_weight1, group_matmul_weight2, group_matmul_weight3]
#     bias = [group_matmul_bias1, group_matmul_bias2, group_matmul_bias3]
#     for i in range(100):
#         torch_npu.npu_grouped_matmul(x, weight, bias=bias, group_list=group_list, split_item=split_item, group_type=-1)


# def run_mix_moe_init_routing():
#     # mix aiv moe_init_routing
#     active_num = 3
#     for i in range(50):
#         torch_npu.npu_moe_init_routing(moe_init_routing_x, moe_init_routing_row_idx,
#                                        moe_init_routing_expert_idx, active_num)


def run_matmul():
    # matmul
    for i in range(1000000):
        torch.matmul(aic_t1, aic_t2)


def run():
    print(f"start, {LOCAL_RANK}")

    for _ in range(3):
        run_matmul()
        run_add()
        # run_mix_group_matmul()
        #run_mix_moe_init_routing()

        # run_all_reduce()
        # run_all_gather()
        # run_mc2()

    print(f"end, {LOCAL_RANK}")

# 定义模型Model
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
    def forward(self):
        run()
        return torch.tensor(0)

#def main():
#    # prof.start()
#    model = Model().npu()
#    config = CompilerConfig()
#    config.mode = "reduce-overhead"
#    # # config.experimental_config.frozen_parameter = True
#    model = torch.compile(model, backend=torchair.get_npu_backend(compiler_config=config), dynamic=False)
#    
#    model()
#
#    prof.start()
#    model()
#
#    torch_npu.npu.synchronize()
#    prof.stop()

def main():
    s = torch_npu.npu.Stream()
    with torch_npu.npu.stream(s):
        g = torch_npu.npu.NPUGraph()
        g.capture_begin()
        run()
        g.capture_end()
    prof.start()
    for i in range(3):
        g.replay()
        print(f"{i} step!")
    # torch_npu.npu.synchronize()
    prof.stop()


if __name__ == '__main__':
    main()

