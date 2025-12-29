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
import torch
import torch_npu

torch.npu.set_device("npu:0")


def run_matmul():
    aic_t1 = torch.rand((128, 128), requires_grad=False).npu()
    aic_t2 = torch.rand((128, 128), requires_grad=False).npu()
    for i in range(5):
        torch.matmul(aic_t1, aic_t2)


def run():
    print(f"start")
    for _ in range(1):
        run_matmul()
    print(f"end")


def main():
    for i in range(3):
        run()


if __name__ == '__main__':
    main()
