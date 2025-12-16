/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
*/

#pragma once
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <cstdint>
#include <atomic>
#include <mutex>

#include "csrc/include/mspti.h"
#include "activity_buffer_pool.h"

namespace Mspti {
namespace Adapter {
// default buffer size config
constexpr size_t MB = 1024 * 1024;
constexpr size_t DEFAULT_BUFFER_SIZE = 8 * MB;
constexpr size_t MAX_BUFFER_SIZE = 256 * MB;

class MsptiAdapter {
public:
    static MsptiAdapter* GetInstance()
    {
        static MsptiAdapter instance;
        return &instance;
    }

    // Common
    msptiResult Start();
    msptiResult Stop();
    msptiResult FlushAll();
    msptiResult FlushPeriod(uint32_t time);
    msptiResult SetBufferSize(size_t size);

    // Mstx
    msptiResult RegisterMstxCallback(PyObject *mstxCallback);
    msptiResult UnregisterMstxCallback();
    PyObject* GetMstxCallback() const;
    msptiResult EnableDomain(const char* domain);
    msptiResult DisableDomain(const char* domain);

    // Kernel
    msptiResult RegisterKernelCallback(PyObject *kernelCallback);
    msptiResult UnregisterKernelCallback();
    PyObject* GetKernelCallback() const;

    // Hccl
    msptiResult RegisterHcclCallback(PyObject *hcclCallback);
    msptiResult UnregisterHcclCallback();
    PyObject* GetHcclCallback() const;

public:
    // buffer
    size_t bufferSize{DEFAULT_BUFFER_SIZE};
    ActivityBufferPool bufferPool;

private:
    static void UserBufferRequest(uint8_t **buffer, size_t *size, size_t *maxNumRecords);
    static void UserBufferComplete(uint8_t *buffer, size_t size, size_t validSize);
    static void UserBufferConsume(msptiActivity *record);

private:
    msptiSubscriberHandle subscriber_;
    std::mutex mtx_;
    std::atomic<uint32_t> refCnt_{0};

    // kernel callback
    PyObject *kernelCallback_{nullptr};
    // mstx callback
    PyObject *mstxCallback_{nullptr};
    // hccl callback
    PyObject *hcclCallback_{nullptr};
};
} // Adapter
} // Mspti
