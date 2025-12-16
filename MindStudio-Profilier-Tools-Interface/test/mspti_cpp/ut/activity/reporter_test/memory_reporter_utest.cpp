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
#include <atomic>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "csrc/activity/ascend/reporter/memory_reporter.h"
#include "csrc/activity/activity_manager.h"

std::atomic_uint32_t g_memoryRecord{0};
std::atomic_uint32_t g_memsetRecord{0};
std::atomic_uint32_t g_memcpyRecord{0};

void UserBufferRequest(uint8_t **buffer, size_t *size, size_t *maxNumRecords)
{
    constexpr uint32_t bufSize = 2 * 1024 * 1024;
    *buffer = static_cast<uint8_t*>(malloc(bufSize));
    *size = bufSize;
    *maxNumRecords = 0;
}

void ActivityParser(msptiActivity *pRecord)
{
    if (pRecord->kind == MSPTI_ACTIVITY_KIND_MEMORY) {
        g_memoryRecord++;
    } else if (pRecord->kind == MSPTI_ACTIVITY_KIND_MEMSET) {
        g_memsetRecord++;
    } else if (pRecord->kind == MSPTI_ACTIVITY_KIND_MEMCPY) {
        g_memcpyRecord++;
    }
}

void UserBufferComplete(uint8_t *buffer, size_t size, size_t validSize)
{
    if (validSize > 0) {
        msptiActivity *pRecord = nullptr;
        msptiResult status = MSPTI_SUCCESS;
        do {
            status = msptiActivityGetNextRecord(buffer, validSize, &pRecord);
            if (status == MSPTI_SUCCESS) {
                ActivityParser(pRecord);
            } else if (status == MSPTI_ERROR_MAX_LIMIT_REACHED) {
                break;
            }
        } while (1);
    }
    free(buffer);
}

namespace {
class MemoryReporterUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        Mspti::Activity::ActivityManager::GetInstance()->RegisterCallbacks(UserBufferRequest, UserBufferComplete);
        GlobalMockObject::verify();
    }
    virtual void TearDown() {}
};

TEST_F(MemoryReporterUtest, ShouldRetSuccessWhenReportMemoryActivity)
{
    EXPECT_EQ(MSPTI_SUCCESS, Mspti::Activity::ActivityManager::GetInstance()->Register(MSPTI_ACTIVITY_KIND_MEMORY));
    void* devPtr = malloc(8);
    uint64_t size = 128;
    auto instance = Mspti::Reporter::MemoryReporter::GetInstance();
    {
        auto alloc_record = Mspti::Reporter::MemoryRecord(
            &devPtr, size, MSPTI_ACTIVITY_MEMORY_DEVICE, MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_ALLOCATION);
        EXPECT_EQ(MSPTI_SUCCESS, instance->ReportMemoryActivity(alloc_record));
        auto release_record = Mspti::Reporter::MemoryRecord(
            &devPtr, 0, MSPTI_ACTIVITY_MEMORY_DEVICE, MSPTI_ACTIVITY_MEMORY_OPERATION_TYPE_RELEASE);
        EXPECT_EQ(MSPTI_SUCCESS, instance->ReportMemoryActivity(release_record));
    }
    EXPECT_EQ(MSPTI_SUCCESS, Mspti::Activity::ActivityManager::GetInstance()->FlushAll());
    const uint32_t expectCnt = 4;
    EXPECT_EQ(g_memoryRecord, expectCnt);
}

TEST_F(MemoryReporterUtest, ShouldRetSuccessWhenReportMemsetActivity)
{
    EXPECT_EQ(MSPTI_SUCCESS, Mspti::Activity::ActivityManager::GetInstance()->Register(MSPTI_ACTIVITY_KIND_MEMSET));
    uint32_t value = 0;
    uint64_t cnt = 128;
    void* stream = malloc(8);
    auto instance = Mspti::Reporter::MemoryReporter::GetInstance();
    {
        auto sync_record = Mspti::Reporter::MemsetRecord(value, cnt, nullptr, 0);
        EXPECT_EQ(MSPTI_SUCCESS, instance->ReportMemsetActivity(sync_record));
        auto async_record = Mspti::Reporter::MemsetRecord(value, cnt, stream, 1);
        EXPECT_EQ(MSPTI_SUCCESS, instance->ReportMemsetActivity(async_record));
    }
    EXPECT_EQ(MSPTI_SUCCESS, Mspti::Activity::ActivityManager::GetInstance()->FlushAll());
    const uint32_t expectCnt = 4;
    EXPECT_EQ(g_memsetRecord, expectCnt);
}

TEST_F(MemoryReporterUtest, ShouldRetSuccessWhenReportMemcpyActivity)
{
    EXPECT_EQ(MSPTI_SUCCESS, Mspti::Activity::ActivityManager::GetInstance()->Register(MSPTI_ACTIVITY_KIND_MEMCPY));
    uint64_t size = 128;
    void* stream = malloc(8);
    auto instance = Mspti::Reporter::MemoryReporter::GetInstance();
    {
        auto sync_record = Mspti::Reporter::MemcpyRecord(RT_MEMCPY_HOST_TO_DEVICE, size, nullptr, 0);
        EXPECT_EQ(MSPTI_SUCCESS, instance->ReportMemcpyActivity(sync_record));
        auto async_record = Mspti::Reporter::MemcpyRecord(RT_MEMCPY_DEVICE_TO_HOST, size, stream, 1);
        EXPECT_EQ(MSPTI_SUCCESS, instance->ReportMemcpyActivity(async_record));
    }
    EXPECT_EQ(MSPTI_SUCCESS, Mspti::Activity::ActivityManager::GetInstance()->FlushAll());
    const uint32_t expectCnt = 4;
    EXPECT_EQ(g_memcpyRecord, expectCnt);
}
}
