/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mspti_adpter.h
 * Description        : Mspti python adapter.
 * Author             : msprof team
 * Creation Date      : 2024/11/19
 * *****************************************************************************
*/

#pragma once
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <cstdint>
#include <atomic>
#include <mutex>

#include "external/mspti.h"

namespace Mspti {
namespace Adapter {
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

    // Mstx
    msptiResult RegisterMstxCallback(PyObject *mstxCallback);
    msptiResult UnregisterMstxCallback();
    PyObject* GetMstxCallback() const;

    // Kernel
    msptiResult RegisterKernelCallback(PyObject *kernelCallback);
    msptiResult UnregisterKernelCallback();
    PyObject* GetKernelCallback() const;

private:
    static void UserBufferRequest(uint8_t **buffer, size_t *size, size_t *maxNumRecords);
    static void UserBufferComplete(uint8_t *buffer, size_t size, size_t validSize);
    static void UserBufferConsume(msptiActivity *record);
    static std::atomic<uint32_t> allocCnt;

private:
    msptiSubscriberHandle subscriber_;
    std::mutex mtx_;
    std::atomic<uint32_t> refCnt_{0};

    // kernel callback
    PyObject *kernelCallback_{nullptr};
    // mstx callback
    PyObject *mstxCallback_{nullptr};
};
} // Adapter
} // Mspti
