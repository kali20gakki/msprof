/**
* @file mspti_adapter_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/
#include <fstream>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "activity/activity_manager.h"
#include "activity/ascend/dev_task_manager.h"
#include "whl/csrc/mspti_adapter.h"
#include "common/inject/mstx_inject.h"

namespace {
using namespace Mspti::Adapter;
const char* PYTHON_FILE = "./mspti_callback.py";
const size_t BUFFER_SIZE = 4;

class MsptiAdapterUtest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        std::ofstream file(PYTHON_FILE);
        file << "def kernel_data_callback(data): print(data)" << std::endl;
        file << "def marker_data_callback(data): print(data)" << std::endl;
        file << "def hccl_data_callback(data): print(data)" << std::endl;
        file.close();
        Py_Initialize();
    }

    static void TearDownTestCase()
    {
        std::remove(PYTHON_FILE);
        Py_Finalize();
        GlobalMockObject::verify();
    }

    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
    virtual void TearDown() {}
};

PyObject *GetMsptiPyCallback(const char* cb_name)
{
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('./')");
    auto pModule = PyImport_ImportModule("mspti_callback");
    return PyObject_GetAttrString(pModule, cb_name);
}

TEST_F(MsptiAdapterUtest, StartStopWillSuccess)
{
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->Start());
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->Start());
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->Stop());
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->Stop());
}

TEST_F(MsptiAdapterUtest, FlushPeriodWillSuccess)
{
    MOCKER_CPP(&Mspti::Activity::ActivityManager::FlushPeriod)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));
    const uint32_t timeMs = 1000;
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->FlushPeriod(timeMs));
}

TEST_F(MsptiAdapterUtest, SetBuferSizeFailedWhenExceedLimit)
{
    const size_t size = Mspti::Adapter::MAX_BUFFER_SIZE / Mspti::Adapter::MB + 1;
    EXPECT_EQ(MSPTI_ERROR_INVALID_PARAMETER, MsptiAdapter::GetInstance()->SetBufferSize(size));
}

TEST_F(MsptiAdapterUtest, SetBuferSizeWillSuccess)
{
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->SetBufferSize(BUFFER_SIZE));
}

TEST_F(MsptiAdapterUtest, RegisterKernelCallbackWillSuccess)
{
    PyObject *kernelCallback = GetMsptiPyCallback("kernel_data_callback");
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->RegisterKernelCallback(kernelCallback));
    EXPECT_EQ(kernelCallback, MsptiAdapter::GetInstance()->GetKernelCallback());
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->UnregisterKernelCallback());
}

TEST_F(MsptiAdapterUtest, PythonKernelCallbackWillRunSuccess)
{
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StartDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StopDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));

    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->SetBufferSize(BUFFER_SIZE));
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->Start());
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityEnable(MSPTI_ACTIVITY_KIND_KERNEL));
    PyObject *kernelCallback = GetMsptiPyCallback("kernel_data_callback");
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->RegisterKernelCallback(kernelCallback));
    auto instance = Mspti::Activity::ActivityManager::GetInstance();
    EXPECT_EQ(MSPTI_SUCCESS, instance->SetDevice(0));
    EXPECT_EQ(MSPTI_SUCCESS, instance->ResetAllDevice());

    constexpr uint64_t timeStamp = 1614659207688700;
    constexpr uint32_t streamId = 3;
    msptiActivityKernel kernel;
    kernel.kind = MSPTI_ACTIVITY_KIND_KERNEL;
    kernel.start = timeStamp;
    kernel.end = timeStamp;
    kernel.ds.deviceId = 0;
    kernel.ds.streamId = streamId;
    kernel.correlationId = 1;
    kernel.type = "KERNEL_AIVEC";
    kernel.name = "Kernel";
    instance->Record(reinterpret_cast<msptiActivity*>(&kernel), sizeof(kernel));

    // invalid kind
    kernel.kind = MSPTI_ACTIVITY_KIND_INVALID;
    instance->Record(reinterpret_cast<msptiActivity*>(&kernel), sizeof(kernel));

    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->Stop());
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->FlushAll());
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->UnregisterKernelCallback());
}

TEST_F(MsptiAdapterUtest, RegisterMstxCallbackWillSuccess)
{
    PyObject *mstxCallback = GetMsptiPyCallback("marker_data_callback");
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->RegisterMstxCallback(mstxCallback));
    EXPECT_EQ(mstxCallback, MsptiAdapter::GetInstance()->GetMstxCallback());
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->UnregisterMstxCallback());
}

TEST_F(MsptiAdapterUtest, PythonMstxCallbackWillRunSuccess)
{
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StartDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StopDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));

    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->SetBufferSize(BUFFER_SIZE));
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->Start());
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityEnable(MSPTI_ACTIVITY_KIND_MARKER));
    PyObject *mstxCallback = GetMsptiPyCallback("marker_data_callback");
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->RegisterMstxCallback(mstxCallback));
    auto instance = Mspti::Activity::ActivityManager::GetInstance();
    EXPECT_EQ(MSPTI_SUCCESS, instance->SetDevice(0));
    EXPECT_EQ(MSPTI_SUCCESS, instance->ResetAllDevice());

    constexpr uint64_t timeStamp = 1614659207688700;
    msptiActivityMarker marker;
    marker.kind = MSPTI_ACTIVITY_KIND_MARKER;
    marker.sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_HOST;
    marker.timestamp = timeStamp;
    marker.id = 0;
    marker.objectId.pt.processId = 0;
    marker.objectId.pt.threadId = 0;
    marker.name = "UserMark";
    marker.domain = "TestCase";
    instance->Record(reinterpret_cast<msptiActivity*>(&marker), sizeof(marker));

    marker.sourceKind = MSPTI_ACTIVITY_SOURCE_KIND_DEVICE;
    instance->Record(reinterpret_cast<msptiActivity*>(&marker), sizeof(marker));

    // invalid kind
    marker.kind = MSPTI_ACTIVITY_KIND_INVALID;
    instance->Record(reinterpret_cast<msptiActivity*>(&marker), sizeof(marker));

    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->Stop());
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->FlushAll());
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->UnregisterMstxCallback());
}

TEST_F(MsptiAdapterUtest, RegisterHcclCallbackWillSuccess)
{
    PyObject *hcclCallback = GetMsptiPyCallback("hccl_data_callback");
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->RegisterHcclCallback(hcclCallback));
    EXPECT_EQ(hcclCallback, MsptiAdapter::GetInstance()->GetHcclCallback());
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->UnregisterHcclCallback());
}

TEST_F(MsptiAdapterUtest, PythonHcclCallbackWillRunSuccess)
{
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StartDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StopDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));

    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->SetBufferSize(BUFFER_SIZE));
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->Start());
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityEnable(MSPTI_ACTIVITY_KIND_HCCL));
    PyObject *hcclCallback = GetMsptiPyCallback("hccl_data_callback");
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->RegisterHcclCallback(hcclCallback));
    auto instance = Mspti::Activity::ActivityManager::GetInstance();
    EXPECT_EQ(MSPTI_SUCCESS, instance->SetDevice(0));
    EXPECT_EQ(MSPTI_SUCCESS, instance->ResetAllDevice());

    constexpr uint64_t start = 1614659207688700;
    constexpr uint64_t end = 1614659207688710;
    msptiActivityHccl hccl;
    hccl.kind = MSPTI_ACTIVITY_KIND_HCCL;
    hccl.start = start;
    hccl.end = end;
    hccl.ds.deviceId = 0;
    hccl.ds.streamId = 0;
    hccl.name = "UserMark";
    hccl.commName = "TestCase";
    hccl.bandWidth = 0;
    instance->Record(reinterpret_cast<msptiActivity*>(&hccl), sizeof(hccl));

    // invalid kind
    hccl.kind = MSPTI_ACTIVITY_KIND_INVALID;
    instance->Record(reinterpret_cast<msptiActivity*>(&hccl), sizeof(hccl));

    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->Stop());
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->FlushAll());
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->UnregisterHcclCallback());
}

TEST_F(MsptiAdapterUtest, ConsumeFailedWhenNotRegisterCallback)
{
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StartDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StopDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));

    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->SetBufferSize(BUFFER_SIZE));
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->Start());
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityEnable(MSPTI_ACTIVITY_KIND_MARKER));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityEnable(MSPTI_ACTIVITY_KIND_KERNEL));
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityEnable(MSPTI_ACTIVITY_KIND_HCCL));
    auto instance = Mspti::Activity::ActivityManager::GetInstance();
    EXPECT_EQ(MSPTI_SUCCESS, instance->SetDevice(0));
    EXPECT_EQ(MSPTI_SUCCESS, instance->ResetAllDevice());

    msptiActivityMarker marker;
    marker.kind = MSPTI_ACTIVITY_KIND_MARKER;
    instance->Record(reinterpret_cast<msptiActivity*>(&marker), sizeof(marker));
    msptiActivityKernel kernel;
    kernel.kind = MSPTI_ACTIVITY_KIND_KERNEL;
    instance->Record(reinterpret_cast<msptiActivity*>(&kernel), sizeof(kernel));
    msptiActivityHccl hccl;
    hccl.kind = MSPTI_ACTIVITY_KIND_HCCL;
    instance->Record(reinterpret_cast<msptiActivity*>(&hccl), sizeof(hccl));

    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->Stop());
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->FlushAll());
}

TEST_F(MsptiAdapterUtest, DisableDefaultDomainBeforeMark)
{
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StartDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));
    MOCKER_CPP(&Mspti::Ascend::DevTaskManager::StopDevProfTask)
        .stubs()
        .will(returnValue(MSPTI_SUCCESS));

    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->Start());
    EXPECT_EQ(MSPTI_SUCCESS, msptiActivityEnable(MSPTI_ACTIVITY_KIND_MARKER));
    auto instance = Mspti::Activity::ActivityManager::GetInstance();
    EXPECT_EQ(MSPTI_SUCCESS, instance->SetDevice(0));
    EXPECT_EQ(MSPTI_SUCCESS, instance->ResetAllDevice());
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->DisableDomain("default"));
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->EnableDomain("default"));
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->FlushAll());
    EXPECT_EQ(MSPTI_SUCCESS, MsptiAdapter::GetInstance()->Stop());
}
}
