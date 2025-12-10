import os
import datetime

import numpy as np
import torch
import torch_npu

CALCULATE_DEVICE = "npu:7"
device = torch.device(CALCULATE_DEVICE)
torch.npu.set_device(device)


def run_matmul():
    aic_t1 = torch.rand((128, 128), requires_grad=False).npu()
    aic_t2 = torch.rand((128, 128), requires_grad=False).npu()
    for i in range(5):
        torch.matmul(aic_t1, aic_t2)


def run():
    print(f"start")

    for _ in range(1):
        run_matmul()
        # run_add()

    print(f"end")


def main():
    for i in range(3):
        run()



if __name__ == '__main__':
    main()

