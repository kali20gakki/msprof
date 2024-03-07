/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : host_trace_worker_utest.cpp
 * Description        : HostTraceWorker UT
 * Author             : msprof team
 * Creation Date      : 2024/1/17
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/worker/host_trace_worker.h"
#include "analysis/csrc/parser/host/cann/event_grouper.h"

using namespace Analysis::Worker;
using namespace Analysis::Utils;
using namespace Analysis::Parser::Host::Cann;

const std::string TEST_HOST_FILE_PATH = "./";

class HostTraceWorkerUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};


TEST_F(HostTraceWorkerUtest, TestKernalHostTraceWorkerShouldReturnTrueWhenInputDirIsEmpty)
{
    HostTraceWorker hostTraceWorker{TEST_HOST_FILE_PATH};
    auto ret = hostTraceWorker.Run();
    EXPECT_EQ(ret, true);
}

TEST_F(HostTraceWorkerUtest, TestKernalHostTraceWorkerShouldReturnTrueWhenGroupResultIsNotEmpty)
{
    HostTraceWorker hostTraceWorker{TEST_HOST_FILE_PATH};
    CANNWarehouses mockWarehouses;
    mockWarehouses.Insert(0, CANNWarehouse{});
    MOCKER_CPP(&EventGrouper::GetGroupEvents).stubs().will(returnValue(mockWarehouses));
    auto ret = hostTraceWorker.Run();
    EXPECT_EQ(ret, true);
}