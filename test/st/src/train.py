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
import argparse
import os


def resnet50_train():
    import torch
    from src.model import ResNet50

    trainer = ResNet50(num_classes=10)
    fake_images = torch.randn(80, 3, 224, 224)
    fake_labels = torch.randint(0, 10, (80,))
    dataset = torch.utils.data.TensorDataset(fake_images, fake_labels)
    loader = torch.utils.data.DataLoader(dataset, batch_size=8, shuffle=True)
    trainer.train(loader, epochs=2, lr=1e-3, freeze_backbone=True)


def communication_train():
    from src.communication_train import train as communication_train_impl

    communication_train_impl()


def acl_graph_train(prof_path: str):
    import torch
    import torch_npu

    n = 1024
    x0 = torch.rand((n,), dtype=torch.float32).npu()
    x1 = torch.rand((n,), dtype=torch.float32).npu()
    y0 = torch.empty_like(x0)
    a0 = torch.rand((32, 32), dtype=torch.float16).npu()
    a1 = torch.rand((32, 32), dtype=torch.float16).npu()
    b0 = torch.empty((32, 32), dtype=torch.float16).npu()

    os.makedirs(prof_path, exist_ok=True)
    stream = torch_npu.npu.Stream()
    graph = torch_npu.npu.NPUGraph()

    with torch_npu.npu.stream(stream):
        graph.capture_begin()
        for _ in range(10):
            torch.add(x0, x1, out=y0)
        for _ in range(10):
            torch.matmul(a0, a1, out=b0)
        graph.capture_end()

    experimental_config = torch_npu.profiler._ExperimentalConfig(
        profiler_level=torch_npu.profiler.ProfilerLevel.Level1,
        data_simplification=False,
    )
    with torch_npu.profiler.profile(
        activities=[
            torch_npu.profiler.ProfilerActivity.CPU,
            torch_npu.profiler.ProfilerActivity.NPU,
        ],
        schedule=torch_npu.profiler.schedule(
            wait=0,
            warmup=1,
            active=2,
            repeat=1,
            skip_first=1
        ),
        on_trace_ready=torch_npu.profiler.tensorboard_trace_handler(prof_path),
        experimental_config=experimental_config
    ) as prof:
        for _ in range(4):
            with torch_npu.npu.stream(stream):
                graph.replay()
            stream.synchronize()
            prof.step()


def torchair_add_train(prof_path: str):
    import torch
    import torch_npu
    import torchair

    os.makedirs(prof_path, exist_ok=True)
    config = torchair.CompilerConfig()
    config.mode = "npugraph_ex"
    npu_backend = torchair.get_npu_backend(compiler_config=config)

    class Model(torch.nn.Module):
        def __init__(self):
            super().__init__()

        def forward(self, x, y):
            return torch.add(x, y)

    model = Model().npu()
    model = torch.compile(model, backend=npu_backend, dynamic=False)
    x = torch.randn(2, 2).npu()
    y = torch.randn(2, 2).npu()

    experimental_config = torch_npu.profiler._ExperimentalConfig(
        profiler_level=torch_npu.profiler.ProfilerLevel.Level1,
        data_simplification=False,
    )
    with torch_npu.profiler.profile(
            activities=[
                torch_npu.profiler.ProfilerActivity.CPU,
                torch_npu.profiler.ProfilerActivity.NPU,
            ],
            schedule=torch_npu.profiler.schedule(
                wait=0,
                warmup=0,
                active=2,
                repeat=1,
                skip_first=1
            ),
            on_trace_ready=torch_npu.profiler.tensorboard_trace_handler(prof_path),
            experimental_config=experimental_config
    ) as prof:
        for _ in range(20):
            model(x, y)
            torch_npu.npu.synchronize()
            prof.step()


# Map function names to actual functions
TRAIN_FUNCS = {
    "resnet50": resnet50_train,
    "communication": communication_train,
    "acl_graph": acl_graph_train,
    "torchair_add": torchair_add_train,
}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Train different models.")
    parser.add_argument(
        "--model", type=str, default="resnet50", choices=TRAIN_FUNCS.keys(), help="Model to train (default: resnet50)"
    )
    parser.add_argument("--prof-path", type=str, default="", help="Profiler output path for models that require it")
    args = parser.parse_args()

    print(f"[INFO] Starting training for model: {args.model}")
    if args.model in {"acl_graph", "torchair_add"}:
        if not args.prof_path:
            raise ValueError(f"--prof-path is required when model is '{args.model}'")
        TRAIN_FUNCS[args.model](args.prof_path)
    else:
        TRAIN_FUNCS[args.model]()
