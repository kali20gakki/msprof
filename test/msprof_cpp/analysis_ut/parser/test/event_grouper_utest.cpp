/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : event_grouper_utest.cpp
 * Description        : EventGrouper UT
 * Author             : msprof team
 * Creation Date      : 2023/11/27
 * *****************************************************************************
 */
#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include "gtest/gtest.h"
#include "utils.h"
#include "log.h"
#include "file.h"
#include "event_grouper.h"
#include "prof_common.h"
#include "fake_trace_generator.h"

using namespace Analysis::Utils;
using namespace Analysis::Parser::Host::Cann;

class EventGrouperUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

void GenApiEventsBin(const std::string &fakeDataDir, const int num)
{
    std::vector<MsprofApi> apiTrace;
    const int n = 2;
    for (int i = n; i < num + n; i++) {
        auto tmp = MsprofApi{};
        tmp.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        tmp.level = MSPROF_REPORT_MODEL_LEVEL;
        tmp.type = static_cast<uint32_t>(EventType::EVENT_TYPE_API);
        tmp.threadId = static_cast<uint32_t>(i);
        tmp.reserve = static_cast<uint32_t>(i);
        tmp.beginTime = static_cast<uint64_t>(n * i);
        tmp.endTime = static_cast<uint64_t>(n * i + 1);
        tmp.itemId = static_cast<uint32_t>(i);

        apiTrace.emplace_back(tmp);
    }

    auto fakeGen = std::make_shared<FakeTraceGenerator>(fakeDataDir);
    fakeGen->WriteBin<MsprofApi>(apiTrace, EventType::EVENT_TYPE_API, true, 0);
}

void GenAdditionalInfoEventsBin(const std::string &fakeDataDir, EventType type, uint16_t level, const int num)
{
    const int size = 8;
    const int n = 2;
    uint8_t fakeData[size];
    std::vector<MsprofAdditionalInfo> tarces;
    for (int i = n; i < num + n; i++) {
        auto tmp = MsprofAdditionalInfo{};
        tmp.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        tmp.level = level;
        tmp.type = static_cast<uint32_t>(type);
        tmp.threadId = static_cast<uint32_t>(i);
        tmp.dataLen = size;
        tmp.timeStamp = static_cast<uint64_t>(n * i + 1);
        memcpy(tmp.data, fakeData, size);

        tarces.emplace_back(tmp);
    }

    auto fakeGen = std::make_shared<FakeTraceGenerator>(fakeDataDir);
    fakeGen->WriteBin<MsprofAdditionalInfo>(tarces, type, true, 0);
}

void GenCompactInfoEventsBin(const std::string &fakeDataDir, EventType type, uint16_t level, const int num)
{
    const int size = 8;
    const int n = 2;
    std::vector<MsprofCompactInfo> tarces;
    uint8_t fakeData[size];

    for (int i = n; i < num + n; i++) {
        auto tmp = MsprofCompactInfo{};
        tmp.magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        tmp.level = level;
        tmp.type = static_cast<uint32_t>(type);
        tmp.threadId = static_cast<uint32_t>(i);
        tmp.dataLen = size;
        tmp.timeStamp = static_cast<uint64_t>(n * i + 1);

        if (type == EventType::EVENT_TYPE_NODE_BASIC_INFO) {
            tmp.data.nodeBasicInfo = MsprofNodeBasicInfo{};
        } else if (type == EventType::EVENT_TYPE_TASK_TRACK) {
            tmp.data.runtimeTrack = MsprofRuntimeTrack{};
        } else {
            memcpy(tmp.data.info, fakeData, size);
        }
        tarces.emplace_back(tmp);
    }

    auto fakeGen = std::make_shared<FakeTraceGenerator>(fakeDataDir);
    fakeGen->WriteBin<MsprofCompactInfo>(tarces, type, true, 0);
}

uint64_t g_getCannEventsNum(std::set<uint32_t> &tids, CANNWarehouses &cwhs, const std::string &eventName)
{
    uint64_t cnt = 0;
    for (auto tid: tids) {
        if (eventName == "kernelEvents" && cwhs[tid].kernelEvents != nullptr) {
            cnt += cwhs[tid].kernelEvents->GetSize();
        } else if (eventName == "graphIdMapEvents" && cwhs[tid].graphIdMapEvents != nullptr) {
            cnt += cwhs[tid].graphIdMapEvents->GetSize();
        } else if (eventName == "fusionOpInfoEvents" && cwhs[tid].fusionOpInfoEvents != nullptr) {
            cnt += cwhs[tid].fusionOpInfoEvents->GetSize();
        } else if (eventName == "nodeBasicInfoEvents" && cwhs[tid].nodeBasicInfoEvents != nullptr) {
            cnt += cwhs[tid].nodeBasicInfoEvents->GetSize();
        } else if (eventName == "tensorInfoEvents" && cwhs[tid].tensorInfoEvents != nullptr) {
            cnt += cwhs[tid].tensorInfoEvents->GetSize();
        } else if (eventName == "contextIdEvents" && cwhs[tid].contextIdEvents != nullptr) {
            cnt += cwhs[tid].contextIdEvents->GetSize();
        } else if (eventName == "hcclInfoEvents" && cwhs[tid].hcclInfoEvents != nullptr) {
            cnt += cwhs[tid].hcclInfoEvents->GetSize();
        } else if (eventName == "taskTrackEvents" && cwhs[tid].taskTrackEvents != nullptr) {
            cnt += cwhs[tid].taskTrackEvents->GetSize();
        }
    }
    return cnt;
}

// 测试输入空文件夹
TEST_F(EventGrouperUTest, TestGroupShouldReturnEmptyWhenDataDirEmpty)
{
    const std::string fakeDataDir = "./fakeDataEmpty";
    File::CreateDir(fakeDataDir);

    auto grouper = std::make_shared<EventGrouper>(fakeDataDir);
    grouper->Group();
    auto tids = grouper->GetThreadIdSet();
    EXPECT_EQ(0, tids.size());
    EXPECT_EQ(true, File::RemoveDir(fakeDataDir, 0));
}

// 测试文件夹中只有Api类型数据
TEST_F(EventGrouperUTest, TestGroupShouldReturnEmptyWhenDataOnlyHasApiBin)
{
    const std::string fakeDataDir = "./fakeDataApi";
    File::RemoveDir(fakeDataDir, 0);

    const std::string hostDataDir = fakeDataDir + "/host/data";
    const int MaxNum = 10000;
    GenApiEventsBin(fakeDataDir, MaxNum);
    auto grouper = std::make_shared<EventGrouper>(hostDataDir);
    grouper->Group();
    auto tids = grouper->GetThreadIdSet();
    auto res = grouper->GetGroupEvents();
    EXPECT_EQ(MaxNum, tids.size());
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "kernelEvents"));
    EXPECT_EQ(true, File::RemoveDir(fakeDataDir, 0));
}

// 测试输入所有类别二进制文件
TEST_F(EventGrouperUTest, TestGroupShouldGroupCorrespondingEventsWhenDataDirHasAllTypeBin)
{
    const std::string fakeDataDir = "./fakeData";
    File::RemoveDir(fakeDataDir, 0);
    const int MaxNum = 10000;
    const std::string hostDataDir = fakeDataDir + "/host/data";
    // 生成所有类型Fake数据
    GenApiEventsBin(fakeDataDir, MaxNum);
    GenAdditionalInfoEventsBin(fakeDataDir,
                               EventType::EVENT_TYPE_GRAPH_ID_MAP,
                               MSPROF_REPORT_MODEL_LEVEL,
                               MaxNum);
    GenAdditionalInfoEventsBin(fakeDataDir,
                               EventType::EVENT_TYPE_FUSION_OP_INFO,
                               MSPROF_REPORT_MODEL_LEVEL,
                               MaxNum);
    GenCompactInfoEventsBin(fakeDataDir,
                            EventType::EVENT_TYPE_NODE_BASIC_INFO,
                            MSPROF_REPORT_NODE_LEVEL,
                            MaxNum);
    GenAdditionalInfoEventsBin(fakeDataDir,
                               EventType::EVENT_TYPE_TENSOR_INFO,
                               MSPROF_REPORT_HCCL_NODE_LEVEL,
                               MaxNum);
    GenAdditionalInfoEventsBin(fakeDataDir,
                               EventType::EVENT_TYPE_CONTEXT_ID,
                               MSPROF_REPORT_NODE_LEVEL,
                               MaxNum);
    GenAdditionalInfoEventsBin(fakeDataDir,
                               EventType::EVENT_TYPE_HCCL_INFO,
                               MSPROF_REPORT_HCCL_NODE_LEVEL,
                               MaxNum);
    GenCompactInfoEventsBin(fakeDataDir,
                            EventType::EVENT_TYPE_TASK_TRACK,
                            MSPROF_REPORT_RUNTIME_LEVEL,
                            MaxNum);

    // 多线程解析
    auto grouper = std::make_shared<EventGrouper>(hostDataDir);
    grouper->Group();
    auto tids = grouper->GetThreadIdSet();
    auto res = grouper->GetGroupEvents();
    // 检查每一个类型Event的数量是否符合预期
    EXPECT_EQ(MaxNum, tids.size());

    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "kernelEvents"));
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "graphIdMapEvents"));
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "fusionOpInfoEvents"));
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "nodeBasicInfoEvents"));
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "tensorInfoEvents"));
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "contextIdEvents"));
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "hcclInfoEvents"));
    EXPECT_EQ(MaxNum, g_getCannEventsNum(tids, res, "taskTrackEvents"));

    EXPECT_EQ(true, File::RemoveDir(fakeDataDir, 0));
}