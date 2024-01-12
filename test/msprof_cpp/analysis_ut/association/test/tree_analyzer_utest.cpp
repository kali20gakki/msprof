/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : tree_analyzer_utest.cpp
 * Description        : TreeAnalyzer UT
 * Author             : msprof team
 * Creation Date      : 2023/11/20
 * *****************************************************************************
 */
#include <string>
#include <memory>
#include <vector>
#include <queue>
#include <iostream>
#include "gtest/gtest.h"
#include "analysis/csrc/entities/event.h"
#include "analysis/csrc/entities/tree.h"
#include "test/msprof_cpp/analysis_ut/fake/fake_trace_generator.h"
#include "analysis/csrc/parser/host/cann/cann_warehouse.h"
#include "analysis/csrc/association/cann/tree_builder.h"
#include "analysis/csrc/association/cann/tree_analyzer.h"
#include "collector/inc/toolchain/prof_common.h"

using namespace Analysis::Parser::Host::Cann;
using namespace Analysis::Association::Cann;
using namespace Analysis::Entities;
using namespace Analysis::Utils;

const std::string TEST_FILE = "./aging.additional.type_info_dic.slice_0";

class TreeAnalyzerUTest : public testing::Test {
protected:
    // 所有测试用例之前执行
    static void SetUpTestCase()
    {
        // 创建test_file文件
        FileWriter fw(TEST_FILE);
        fw.WriteText("5000_5000:EVENT_RECORD\n"); // Runtime level hccl task
        fw.WriteText("5000_4000:FFTS_PLUS\n");    // Runtime level ffts+ task
        fw.WriteText("5000_3000:STARS_COMMON\n"); // Runtime level ComputeTask
        fw.WriteText("5000_2000:KERNEL\n");       // Runtime level ComputeTask
        fw.WriteText("5000_1000:MEMCPY_ASYNC\n"); // Runtime level OtherTask
        fw.Close();
    }
    // 所有测试用例之后执行
    static void TearDownTestCase()
    {
        File::DeleteFile(TEST_FILE);  // 删除test_file文件
    }
};

// 生成建树所需的三层节点
std::shared_ptr<EventQueue> GenKernelEventQueue()
{
    auto kernelEvents = std::make_shared<EventQueue>(1, 100);
    std::vector<std::string> levels{"Model", "Node", "Hccl"};
    std::unordered_map<std::string, uint16_t> levelNum{
        {"Model", MSPROF_REPORT_MODEL_LEVEL},
        {"Node", MSPROF_REPORT_NODE_LEVEL},
        {"Hccl", MSPROF_REPORT_HCCL_NODE_LEVEL},
    };
    // Model :  [1,            200]  [201,             400]  [401, 800]
    // Node  :  [2, 99]  [100, 150]  [202,             399]
    // Hccl  :                       [210, 250], [252, 300]
    std::unordered_map<std::string, std::vector<std::pair<uint64_t, uint64_t>>> apiEvents{
        {"Model", {{1, 200}, {201, 400}, {401, 800}}},
        {"Node", {{2, 99}, {100, 150}, {202, 399}}},
        {"Hccl", {{210, 250}, {252, 300}}},
    };

    for (const auto &le: levels) {
        for (auto range: apiEvents[le]) {
            FakeEventGenerator::AddApiEvent(kernelEvents, levelNum[le], range.first, range.second);
        }
    }
    kernelEvents->Sort();
    return kernelEvents;
}

std::shared_ptr<EventQueue> GenHcclInfoEventQueueL1()
{
    auto hcclInfoEvents = std::make_shared<EventQueue>(1, 10);
    std::unordered_map<std::string, std::vector<uint64_t>> Events{
        {"Hccl", {230, 260}}
    };
    for (auto dot: Events["Hccl"]) {
        FakeEventGenerator::AddHcclInfoEvent(hcclInfoEvents, dot);
    }
    hcclInfoEvents->Sort();
    return hcclInfoEvents;
}

std::shared_ptr<EventQueue> GenNodeBasicInfoEventQueueL1()
{
    auto nodeBasicInfoEvents = std::make_shared<EventQueue>(1, 10);
    std::unordered_map<std::string, std::vector<uint64_t>> Events{
        {"Node", {125, 222}}
    };
    for (auto dot: Events["Node"]) {
        FakeEventGenerator::AddNodeBasicEvent(nodeBasicInfoEvents, dot);
    }
    nodeBasicInfoEvents->Sort();
    return nodeBasicInfoEvents;
}

std::shared_ptr<EventQueue> GenCtxIdEventEventQueueL1()
{
    auto contextIdEvents = std::make_shared<EventQueue>(1, 1);
    uint64_t dot1 = 125;
    FakeEventGenerator::AddCtxIdEvent(contextIdEvents, dot1, MSPROF_REPORT_NODE_LEVEL);
    contextIdEvents->Sort();
    return contextIdEvents;
}

std::shared_ptr<EventQueue> GenTaskTrackEventQueue()
{
    auto taskTrackEvents = std::make_shared<EventQueue>(1, 10);
    // Model :  [1,            200]  [201,             400]  [401, 800]
    // Node  :  [2, 99]  [100, 150]  [202,             399]
    // Hccl  :                       [210, 250], [252, 300]
    std::vector<std::pair<uint64_t, uint64_t>> items{
        {50, 1000},  // Runtime OtherTask
        {110, 3000}, // Runtime STARS_COMMON
        {220, 5000}, // Runtime EVENT_RECORD
        {260, 5000}, // Runtime EVENT_RECORD
    };
    for (auto item : items) {
        FakeEventGenerator::AddTaskTrackEvent(taskTrackEvents, item.first, item.second);
    }
    taskTrackEvents->Sort();
    return taskTrackEvents;
}

std::shared_ptr<Event> GenCtxIdEvent(uint64_t dot, uint16_t level, uint32_t ctxIdNum)
{
    EventInfo testInfo{EventType::EVENT_TYPE_CONTEXT_ID, level, dot, dot};
    auto addtionInfo = std::make_shared<MsprofAdditionalInfo>();
    uint32_t dataLen = 2;
    addtionInfo->timeStamp = dot;
    addtionInfo->dataLen = dataLen;

    MsprofContextIdInfo ctxId;
    ctxId.opName = dot;
    ctxId.ctxIdNum = ctxIdNum;
    uint32_t ids[2] = {0, 1};
    std::memcpy(ctxId.ctxIds, &ids, sizeof(ids));
    std::memcpy(addtionInfo->data, &ctxId, sizeof(ctxId));

    auto eventPtr = std::make_shared<Event>(addtionInfo, testInfo);
    return eventPtr;
}

std::shared_ptr<Event> GenHcclInfoEvent(uint64_t dot)
{
    uint32_t cnt = 0;
    EventInfo testInfo{EventType::EVENT_TYPE_HCCL_INFO, MSPROF_REPORT_HCCL_NODE_LEVEL, dot, dot};
    auto hcclInfo = std::make_shared<MsprofAdditionalInfo>();
    hcclInfo->magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
    hcclInfo->level = MSPROF_REPORT_HCCL_NODE_LEVEL;
    hcclInfo->type = static_cast<uint32_t>(EventType::EVENT_TYPE_HCCL_INFO);
    hcclInfo->timeStamp = static_cast<uint64_t>(dot);

    auto hcclTrace = MsprofHcclInfo{};
    hcclTrace.ctxID = cnt;
    hcclTrace.itemId = dot;
    std::memcpy(hcclInfo->data, &hcclTrace, sizeof(hcclTrace));
    auto eventPtr = std::make_shared<Event>(hcclInfo, testInfo);

    return eventPtr;
}

TEST_F(TreeAnalyzerUTest, TestUpdateHcclSmallOpDescsShouldReturn2DescsWhenInputCtxIdLenIs2)
{
    uint64_t dot = 123;
    uint64_t ctxIdNum = 2;
    HCCLSmallOpDescs descs;
    std::vector<std::shared_ptr<Event>> ctxIdRecords{
        GenCtxIdEvent(dot, MSPROF_REPORT_NODE_LEVEL, ctxIdNum),
    };

    std::vector<std::shared_ptr<Event>> hcclInfoRecords{
        GenHcclInfoEvent(dot)
    };

    auto treeNode = std::make_shared<TreeNode>(nullptr);
    auto ana = std::make_shared<TreeAnalyzer>(treeNode, 1);

    auto ret = ana->UpdateHcclSmallOpDescs(descs, ctxIdRecords, hcclInfoRecords);

    std::vector<uint32_t> expectCtxIds{0, 1};
    EXPECT_EQ(ret, true);
    EXPECT_EQ(descs.size(), expectCtxIds.size());

    // 检查ctxIds
    for (auto id : expectCtxIds) {
        auto check = descs.count(id) != 0;
        EXPECT_EQ(check, true);
    }
}

// 测试输入为空指针
TEST_F(TreeAnalyzerUTest, TestTreeAnalyzerShouldReturnZeroWhenInputNullptr)
{
    auto treeNode = std::make_shared<TreeNode>(nullptr);
    auto ana = std::make_shared<TreeAnalyzer>(treeNode, 1);
    ana->Analyze();

    auto computeTasks = ana->GetComputeTasks();
    auto hcclTasks = ana->GetHCCLTasks();
    auto bigOPs = ana->GetHcclBigOps();
    auto tasks = ana->GetTasks();

    EXPECT_EQ(computeTasks.size(), 0);
    EXPECT_EQ(hcclTasks.size(), 0);
    EXPECT_EQ(bigOPs.size(), 0);
    EXPECT_EQ(tasks.size(), 0);
}

// 测试场景1 L0 : 静态图+计算类算子ffts+模式+hccl算子传统模式（MindSpore、Tensorflow、ACL推理）
TEST_F(TreeAnalyzerUTest, TestTreeAnalyzerShouldReturnCorrectNumWhenScenario1L0)
{
    auto kernelEvents = GenKernelEventQueue();
    auto taskTrackEvents = GenTaskTrackEventQueue();

    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    cannWarehouse->kernelEvents = kernelEvents;
    cannWarehouse->taskTrackEvents = taskTrackEvents;

    // 建树
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);
    auto treeNode = treeBuilder->Build();

    // 分析树
    TypeData::GetInstance().Load("./");
    auto ana = std::make_shared<TreeAnalyzer>(treeNode, 1);
    ana->Analyze();

    auto computeTasks = ana->GetComputeTasks();
    auto hcclTasks = ana->GetHCCLTasks();
    auto bigOPs = ana->GetHcclBigOps();
    auto tasks = ana->GetTasks();

    std::vector<uint64_t> expectComputeTasks{110};
    std::vector<uint64_t> expectHcclTasks{220, 260};
    std::vector<uint64_t> expectTasks{50, 110, 220, 260};
    const uint64_t expectbigOPsNum = 1;
    // 先检查个数正确
    EXPECT_EQ(computeTasks.size(), expectComputeTasks.size());
    EXPECT_EQ(hcclTasks.size(), expectHcclTasks.size());
    EXPECT_EQ(bigOPs.size(), expectbigOPsNum);
    EXPECT_EQ(tasks.size(), expectTasks.size());

    // 再匹配内容正确
    for (uint16_t i = 0; i < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i]->timeStamp, expectComputeTasks[i]);
    }
    for (uint16_t i = 0; i < hcclTasks.size(); i++) {
        EXPECT_EQ(hcclTasks[i]->timeStamp, expectHcclTasks[i]);
    }
    for (uint16_t i = 0; i < tasks.size(); i++) {
        EXPECT_EQ(tasks[i]->timeStamp, expectTasks[i]);
    }
}

// 测试场景1 L1 : 静态图+计算类算子ffts+模式+hccl算子传统模式（MindSpore、Tensorflow、ACL推理）
TEST_F(TreeAnalyzerUTest, TestTreeAnalyzerShouldReturnCorrectNumWhenScenario1L1)
{
    auto kernelEvents = GenKernelEventQueue();
    auto hcclInfoEvents = GenHcclInfoEventQueueL1();
    auto nodeBasicInfoEvents = GenNodeBasicInfoEventQueueL1();
    auto contextIdEvents = GenCtxIdEventEventQueueL1();
    auto taskTrackEvents = GenTaskTrackEventQueue();

    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    cannWarehouse->kernelEvents = kernelEvents;
    cannWarehouse->hcclInfoEvents = hcclInfoEvents;
    cannWarehouse->nodeBasicInfoEvents = nodeBasicInfoEvents;
    cannWarehouse->contextIdEvents = contextIdEvents;
    cannWarehouse->taskTrackEvents = taskTrackEvents;

    // 建树
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);
    auto treeNode = treeBuilder->Build();

    // 分析树
    TypeData::GetInstance().Load("./");
    auto ana = std::make_shared<TreeAnalyzer>(treeNode, 1);
    ana->Analyze();

    auto computeTasks = ana->GetComputeTasks();
    auto hcclTasks = ana->GetHCCLTasks();
    auto bigOPs = ana->GetHcclBigOps();
    auto tasks = ana->GetTasks();

    std::vector<uint64_t> expectComputeTasks{110, 110};
    std::vector<uint64_t> expectHcclTasks{220, 260};
    std::vector<uint64_t> expectTasks{50, 110, 110, 220, 260};
    const uint64_t expectbigOPsNum = 1;

    // 先检查个数正确
    EXPECT_EQ(computeTasks.size(), expectComputeTasks.size());
    EXPECT_EQ(hcclTasks.size(), expectHcclTasks.size());
    EXPECT_EQ(bigOPs.size(), expectbigOPsNum);
    EXPECT_EQ(tasks.size(), expectTasks.size());

    // 检查NodeBasicInfo信息
    uint64_t expectOpName1 = 125;
    EXPECT_EQ(computeTasks.back()->op->opDesc->nodeDesc->data.nodeBasicInfo.opName, expectOpName1);
    // 检查HCCLInfo信息
    uint64_t expectOpName2 = 230;
    uint64_t expectOpName3 = 260;
    EXPECT_EQ(hcclTasks.front()->op->name, expectOpName2);
    EXPECT_EQ(hcclTasks.back()->op->name, expectOpName3);
}