/**
 * @file sinh_custom.cpp
 *
 * Copyright (C) 2024. Huawei Technologies Co., Ltd. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "kernel_operator.h"

constexpr int32_t TOTAL_LENGTH = 8 * 2048;                            // total length of data
constexpr int32_t USE_CORE_NUM = 8;                                   // num of core used
constexpr int32_t BLOCK_LENGTH = TOTAL_LENGTH / USE_CORE_NUM;         // length computed of each core
constexpr int32_t TILE_NUM = 8;                                       // split data into 8 tiles for each core
constexpr int32_t BUFFER_NUM = 2;                                     // tensor num for each queue
constexpr int32_t TILE_LENGTH = BLOCK_LENGTH / TILE_NUM / BUFFER_NUM; // separate to 2 parts, due to double buffer

class KernelSinh {
public:
    __aicore__ inline KernelSinh() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR y)
    {
        xGm.SetGlobalBuffer((__gm__ half *)x + BLOCK_LENGTH * AscendC::GetBlockIdx(), BLOCK_LENGTH);
        yGm.SetGlobalBuffer((__gm__ half *)y + BLOCK_LENGTH * AscendC::GetBlockIdx(), BLOCK_LENGTH);
        pipe.InitBuffer(inQueueX, BUFFER_NUM, TILE_LENGTH * sizeof(half));
        pipe.InitBuffer(outQueueY, BUFFER_NUM, TILE_LENGTH * sizeof(half));
        pipe.InitBuffer(tempQueue_1, TILE_LENGTH * sizeof(half));
        pipe.InitBuffer(tempQueue_2, TILE_LENGTH * sizeof(half));
    }
    __aicore__ inline void Process()
    {
        int32_t loopCount = TILE_NUM * BUFFER_NUM;
        for (int32_t i = 0; i < loopCount; i++) {
            CopyIn(i);
            Compute(i);
            CopyOut(i);
        }
    }

private:
    __aicore__ inline void CopyIn(int32_t progress)
    {
        // TODO: 将Global Memory上的输入Tensor搬运到Local Memory
        AscendC::LocalTensor<half> xLocal = inQueueX.AllocTensor<half>();
        DataCopy(xLocal, xGm[progress * TILE_LENGTH], TILE_LENGTH);
        inQueueX.EnQue(xLocal);
    }
    __aicore__ inline void Compute(int32_t progress)
    {
        // TODO: 对Local Memory上的Tensor进行运算，结果存储到输出Tensor
        AscendC::LocalTensor<half> xLocal = inQueueX.DeQue<half>();
        AscendC::LocalTensor<half> yLocal = outQueueY.AllocTensor<half>();

        {
            AscendC::LocalTensor<half> exp_x1 = tempQueue_1.AllocTensor<half>();
            AscendC::LocalTensor<half> exp_x2 = tempQueue_2.AllocTensor<half>();

            Exp(exp_x1, xLocal, TILE_LENGTH);    // exp(x)
            half sign = -1;
            Muls(exp_x2, xLocal, sign, TILE_LENGTH);    // -x
            Exp(exp_x2, exp_x2, TILE_LENGTH);    // exp(-x)
            Sub(yLocal, exp_x1, exp_x2, TILE_LENGTH);    // exp(x) - exp(-x)
            half scalar = 0.5;
            Muls(yLocal, yLocal, scalar, TILE_LENGTH);
            inQueueX.FreeTensor(xLocal);
            tempQueue_1.FreeTensor(exp_x1);
            tempQueue_2.FreeTensor(exp_x2);
        }
        outQueueY.EnQue(yLocal);
    }
    __aicore__ inline void CopyOut(int32_t progress)
    {
        // TODO: 将输出Tensor从Local Memory搬运至Global Memory
        AscendC::LocalTensor<half> yLocal = outQueueY.DeQue<half>();
        DataCopy(yGm[progress * TILE_LENGTH], yLocal, TILE_LENGTH);
        outQueueY.FreeTensor(yLocal);
    }

private:
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::QuePosition::VECIN, BUFFER_NUM> inQueueX;
    AscendC::TQue<AscendC::QuePosition::VECOUT, BUFFER_NUM> outQueueY;
    AscendC::TBuf<AscendC::QuePosition::VECCALC> tempQueue_1;
    AscendC::TBuf<AscendC::QuePosition::VECCALC> tempQueue_2;
    AscendC::GlobalTensor<half> xGm;
    AscendC::GlobalTensor<half> yGm;
};

extern "C" __global__ __aicore__ void sinh_custom(GM_ADDR x, GM_ADDR y)
{
    KernelSinh op;
    op.Init(x, y);
    op.Process();
}

#ifndef ASCENDC_CPU_DEBUG
void sinh_custom_do(uint32_t blockDim, void *stream, uint8_t *x, uint8_t *y)
{
    sinh_custom<<<blockDim, nullptr, stream>>>(x, y);
}
#endif

