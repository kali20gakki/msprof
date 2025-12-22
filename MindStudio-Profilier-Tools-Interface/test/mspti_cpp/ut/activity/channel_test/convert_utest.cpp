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
 
#include <memory>
#include "gtest/gtest.h"
 
#include "mockcpp/mockcpp.hpp"
#include "csrc/common/inject/inject_base.h"
#include "csrc/activity/ascend/channel/channel_data.h"
#include "csrc/activity/ascend/entity/soclog.h"
#include "csrc/activity/ascend/channel/soclog_convert.h"
#include "csrc/common/context_manager.h"
 
namespace {
 
namespace {
    StarsSocLog soclogs[] = {
        {
            .funcType = STARS_FUNC_TYPE_BEGIN,
            .cnt = 5,
            .taskType = 22,
            .resv0 = 0,
            .streamId = 100,
            .taskId = 200,
            .timestamp = 1625151234567890ULL,
            .resv1 = 0,
            .accId = 31,
            .acsqId = 512,
            .resv2 = {0}
        },
        {
            .funcType = STARS_FUNC_TYPE_END,
            .cnt = 3,
            .taskType = 7,
            .resv0 = 0,
            .streamId = 10,
            .taskId = 20,
            .timestamp = 1625159876543210ULL,
            .resv1 = 0,
            .accId = 5,
            .acsqId = 999,
            .resv2 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}
        },
        {
            .funcType = STARS_FUNC_TYPE_BEGIN,
            .cnt = 15,
            .taskType = 63,
            .resv0 = 0xFFFF,
            .streamId = 65535,
            .taskId = 12345,
            .timestamp = 0xFFFFFFFFFFFFFFFFULL,
            .resv1 = 0xAAAA,
            .accId = 63,
            .acsqId = 1023,
            .resv2 = {0xDEAD, 0xBEEF, 0x1234, 0x5678, 0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD, 0xEEEE, 0xFFFF, 0x0}
        }
    };
 
    FftsPlusLog fftsLogs[] = {
        {
            .funcType = FFTS_PLUS_TYPE_START,
            .cnt = 2,
            .taskType = 3,
            .resv2 = 0,
            .streamId = 10,
            .taskId = 20,
            .timestamp = 1234567890123456ULL,
            .subTaskType = 5,
            .resv3 = 0,
            .subTaskId = 100,
            .resv4 = 0,
            .fftsType = 2,
            .threadId = 300,
            .resv5 = {0}
        },
        {
            .funcType = FFTS_PLUS_TYPE_END,
            .cnt = 7,
            .taskType = 11,
            .resv2 = 0,
            .streamId = 20,
            .taskId = 21,
            .timestamp = 22334455667788ULL,
            .subTaskType = 12,
            .resv3 = 0,
            .subTaskId = 101,
            .resv4 = 0,
            .fftsType = 1,
            .threadId = 301,
            .resv5 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
        },
        {
            .funcType = FFTS_PLUS_TYPE_START,
            .cnt = 15,
            .taskType = 63,
            .resv2 = 0xABCD,
            .streamId = 999,
            .taskId = 888,
            .timestamp = 0xFFFFFFFFFFFFFFFFULL,
            .subTaskType = 255,
            .resv3 = 0xFF,
            .subTaskId = 777,
            .resv4 = 0x1FFF,
            .fftsType = 7,
            .threadId = 65535,
            .resv5 = {0xDEAD, 0xBEEF, 0, 1, 2, 3, 4, 5, 6, 7}
        }
    };
 
    StarsSocLogV6 socLogsV6[] = {
        {
            .funcType = STARS_FUNC_TYPE_BEGIN,
            .cnt = 3,
            .taskType = 5,
            .resv0 = 0,
            .taskId = 2,
            .timestamp = 1000000000000ULL,
            .resv1 = 0,
            .accId = 1,
            .acsqId = 2,
            .resv2 = {0, 0, 0}
        },
        {
            .funcType = STARS_FUNC_TYPE_BEGIN,
            .cnt = 8,
            .taskType = 14,
            .resv0 = 0,
            .taskId = 200,
            .timestamp = 5555555555555ULL,
            .resv1 = 0,
            .accId = 12,
            .acsqId = 345,
            .resv2 = {11, 22, 33}
        },
        {
            .funcType = STARS_FUNC_TYPE_END,
            .cnt = 15,
            .taskType = 63,
            .resv0 = 0xAAAA,
            .taskId = 65534,
            .timestamp = 0xFFFFFFFFFFFFFFFFULL,
            .resv1 = 0x1234,
            .accId = 63,
            .acsqId = 1023,
            .resv2 = {0x12345678, 0x9ABCDEF0, 0x0BADF00D}
        }
    };
 
    FftsPlusLog unknow[] = {
        {
            .funcType = 63,
            .cnt = 15,
            .taskType = 63,
            .resv2 = 0xABCD,
            .streamId = 999,
            .taskId = 888,
            .timestamp = 0xFFFFFFFFFFFFFFFFULL,
            .subTaskType = 255,
            .resv3 = 0xFF,
            .subTaskId = 777,
            .resv4 = 0x1FFF,
            .fftsType = 7,
            .threadId = 65535,
            .resv5 = {0xDEAD, 0xBEEF, 0, 1, 2, 3, 4, 5, 6, 7}
        }
    };
}
 
using namespace Mspti;
 
class ConvertUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
    }
    virtual void TearDown() {}
};
 
TEST_F(ConvertUtest, ParseStarsSocLogSuccess)
{
    size_t pos = 0;
    std::vector<uint64_t> expectType{STARS_FUNC_TYPE_BEGIN, STARS_FUNC_TYPE_END, STARS_FUNC_TYPE_BEGIN};
    std::vector<uint64_t> expectTimeStamp{1625151234567890ULL, 1625159876543210ULL, 0xFFFFFFFFFFFFFFFFULL};
    std::vector<HalLogData> halLogs =
        Convert::SocLogConvert::GetInstance().TransData((char*)soclogs, sizeof(StarsSocLog) * 3, 1, pos);
    for (int i = 0; i < expectType.size(); i++) {
        EXPECT_EQ(expectType[i], halLogs[i].acsq.funcType);
        EXPECT_EQ(expectTimeStamp[i], halLogs[i].acsq.timestamp);
    }
}
 
TEST_F(ConvertUtest, ParseFFTSLogSuccess)
{
    size_t pos = 0;
    std::vector<uint64_t> expectType{FFTS_PLUS_TYPE_START, FFTS_PLUS_TYPE_END, FFTS_PLUS_TYPE_START};
    std::vector<uint64_t> expectTimeStamp{1234567890123456ULL, 22334455667788ULL, 0xFFFFFFFFFFFFFFFFULL};
    std::vector<uint64_t> expectSubTaskId{100, 101, 777};
    std::vector<HalLogData> halLogs =
        Convert::SocLogConvert::GetInstance().TransData((char*)fftsLogs, sizeof(FftsPlusLog) * 3, 1, pos);
    for (int i = 0; i < expectType.size(); i++) {
        EXPECT_EQ(expectType[i], halLogs[i].ffts.funcType);
        EXPECT_EQ(expectTimeStamp[i], halLogs[i].ffts.timestamp);
        EXPECT_EQ(expectSubTaskId[i], halLogs[i].ffts.subTaskId);
    }
}
 
TEST_F(ConvertUtest, ParseStarsSocLogV6Success)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Mspti::Common::ContextManager::GetChipType)
        .stubs()
        .will(returnValue(Mspti::Common::PlatformType::CHIP_V6));
    size_t pos = 0;
    std::vector<uint64_t> expectType{STARS_FUNC_TYPE_BEGIN, STARS_FUNC_TYPE_BEGIN, STARS_FUNC_TYPE_END};
    std::vector<uint64_t> expectTimeStamp{1000000000000ULL, 5555555555555ULL, 0xFFFFFFFFFFFFFFFFULL};
    std::vector<HalLogData> halLogs =
        Convert::SocLogConvert::GetInstance().TransData((char*)socLogsV6, sizeof(StarsSocLogV6) * 3, 1, pos);
    for (int i = 0; i < expectType.size(); i++) {
        EXPECT_EQ(expectType[i], halLogs[i].acsq.funcType);
        EXPECT_EQ(expectTimeStamp[i], halLogs[i].acsq.timestamp);
    }
}
 
TEST_F(ConvertUtest, ParseUnknowDataTest)
{
    size_t pos = 0;
    std::vector<HalLogData> halLogs =
        Convert::SocLogConvert::GetInstance().TransData((char*)unknow, sizeof(FftsPlusLog), 1, pos);
    EXPECT_TRUE(halLogs.empty());
}
}