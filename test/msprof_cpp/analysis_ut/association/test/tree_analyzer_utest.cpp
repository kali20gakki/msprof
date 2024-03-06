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
        fw.WriteText("5000_2:EVENT_RECORD\n");    // Runtime level other
        fw.WriteText("5000_52:FFTS_PLUS\n");      // Runtime level ffts+
        fw.WriteText("5000_50:STARS_COMMON\n");   // Runtime level stars common
        fw.WriteText("5000_5:MEMCPY_ASYNC\n");    // Runtime level memcpy
        fw.WriteText("5000_0:KERNEL_AICORE\n");   // Runtime level aicore
        fw.WriteText("5000_1:KERNEL_AICPU\n");   // Runtime level aicpu
        fw.WriteText("5000_1045:NotifyRecord\n"); // Runtime level hccl
        fw.Close();
    }
    // 所有测试用例之后执行
    static void TearDownTestCase()
    {
        File::DeleteFile(TEST_FILE);  // 删除test_file文件
    }
};

struct HostTaskCompCtx {
    bool operator()(std::shared_ptr<HostTask> &t1, std::shared_ptr<HostTask> &t2)
    {
        return (t1->contextId < t2->contextId);
    }
};

// 生成建树所需的三层节点
std::shared_ptr<EventQueue> GenKernelEventQueue(std::unordered_map<
    std::string, std::vector<std::pair<uint64_t, uint64_t>>> &apiEvents)
{
    auto kernelEvents = std::make_shared<EventQueue>(1, 100);
    std::vector<std::string> levels{"Model", "Node", "Hccl"};
    std::unordered_map<std::string, uint16_t> levelNum{
        {"Model", MSPROF_REPORT_MODEL_LEVEL},
        {"Node", MSPROF_REPORT_NODE_LEVEL},
        {"Hccl", MSPROF_REPORT_HCCL_NODE_LEVEL},
    };
    for (const auto &le: levels) {
        for (auto range: apiEvents[le]) {
            FakeEventGenerator::AddApiEvent(kernelEvents, levelNum[le], range.first, range.second);
        }
    }
    kernelEvents->Sort();
    return kernelEvents;
}

// 生成建树所需的三层节点
std::shared_ptr<EventQueue> GenKernelEventQueueWithTradComm()
{
    // Model :  [1,            200]  [201,                                    400]  [401, 800]
    // Node  :  [2, 99]  [100, 150]  [202,            250] [251,  301] [329, 351]
    // Hccl  :                       [210, 220] [230, 250]  [252, 300] [330, 350]
    std::unordered_map<std::string, std::vector<std::pair<uint64_t, uint64_t>>> apiEvents{
        {"Model", {{1, 200}, {201, 400}, {401, 800}}},
        {"Node", {{2, 99}, {100, 150}, {202, 250}, {251, 301}, {329, 351}}},
        {"Hccl", {{210, 220}, {230, 250}, {252, 300}, {330, 350}}},
    };

    return GenKernelEventQueue(apiEvents);
}

// 生成建树所需的三层节点
std::shared_ptr<EventQueue> GenKernelEventQueueWithFFtsComm()
{
    // Model :  [1,            200]  [201,             400]  [401, 800]
    // Node  :  [2, 99]  [100, 150]  [202,  300] [302, 399]
    // Hccl  :                       [202,  300] [302, 399]
    std::unordered_map<std::string, std::vector<std::pair<uint64_t, uint64_t>>> apiEvents{
        {"Model", {{1, 200}, {201, 400}, {401, 800}}},
        {"Node", {{2, 99}, {100, 150}, {160, 195}, {202, 300}, {302, 399}}},
        {"Hccl", {{202, 300}, {302, 399}}},
    };

    return GenKernelEventQueue(apiEvents);
}

std::shared_ptr<EventQueue> GenHcclInfoEventQueue(const std::vector<uint64_t> &dots,
                                                  const std::vector<uint64_t> &ctxIds)
{
    auto hcclInfoEvents = std::make_shared<EventQueue>(1, 10);
    for (uint32_t i = 0; i < dots.size(); i++) {
        FakeEventGenerator::AddHcclInfoEvent(hcclInfoEvents, dots[i], ctxIds[i]);
    }
    hcclInfoEvents->Sort();
    return hcclInfoEvents;
}

// OpDesc represents <dot, op_type, task_type>
std::shared_ptr<EventQueue> GenNodeBasicInfoEventQueue(const std::vector<std::tuple<uint64_t,
                                                                                    uint64_t,
                                                                                    uint32_t>> OpDescs)
{
    auto nodeBasicInfoEvents = std::make_shared<EventQueue>(1, 10);
    const uint16_t dotIndex = 0;
    const uint16_t opTypeIndex = 1;
    const uint16_t taskTypeIndex = 2;
    for (auto &t: OpDescs) {
        FakeEventGenerator::AddNodeBasicEvent(nodeBasicInfoEvents, std::get<dotIndex>(t),
                                              std::get<opTypeIndex>(t), std::get<taskTypeIndex>(t));
    }
    nodeBasicInfoEvents->Sort();
    return nodeBasicInfoEvents;
}

// tensorDesc represents dot
std::shared_ptr<EventQueue> GenTensorInfoEventQueue(const std::vector<uint64_t> &tensorDescs)
{
    auto tensorInfoEvents = std::make_shared<EventQueue>(1, 10);
    for (auto dot: tensorDescs) {
        FakeEventGenerator::AddTensorInfoEvent(tensorInfoEvents, dot);
    }

    tensorInfoEvents->Sort();
    return tensorInfoEvents;
}

std::shared_ptr<EventQueue> GenHcclCtxIdEventQueue()
{
    auto contextIdEvents = std::make_shared<EventQueue>(1, 10);
    std::vector<uint64_t> dots = {230, 310};
    uint32_t endCtxId = 2;
    for (auto &dot :dots) {
        FakeEventGenerator::AddHcclCtxIdEvent(contextIdEvents, dot, endCtxId);
    }
    return contextIdEvents;
}

std::shared_ptr<EventQueue> GenNodeCtxIdEventQueue()
{
    auto contextIdEvents = std::make_shared<EventQueue>(1, 5);
    std::vector<uint64_t> dots = {125, 126, 127, 128};
    for (uint32_t i = 0; i < dots.size(); i++) {
        auto dot = dots[i];
        FakeEventGenerator::AddNodeCtxIdEvent(contextIdEvents, dot, {i * 2, i * 2 + 1});
    }
    contextIdEvents->Sort();
    return contextIdEvents;
}

std::shared_ptr<EventQueue> GenTaskTrackEventQueue(const std::vector<std::pair<uint64_t, uint64_t>> &runtimeDescs)
{
    auto taskTrackEvents = std::make_shared<EventQueue>(1, 10);
    for (auto item : runtimeDescs) {
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

    auto ret = ana->UpdateHcclSmallOpDescs(descs, ctxIdRecords, hcclInfoRecords, 0);

    std::vector<uint32_t> expectCtxIds{0, 1, DEFAULT_CONTEXT_ID};
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

std::shared_ptr<TreeAnalyzer> GetAnalyzerForScenario1L0()
{
    // 逻辑树如下，其中[n]标识每个context id event中带几个context id
    // Model :  [1,            200]  [201,                                    400]  [401, 800]
    // Node  :  [2, 99]  [100, 150]  [202,            250] [251,  301] [329, 351]
    //                  c(125,126)[2]
    //                  c(127,128)[2]
    // Hccl  :                       [210, 220] [230, 250]  [252, 300] [330, 350]
    // Runtime:  tk(50)    tk(110)     tk(220)    tk(240)     tk(260)    tk(340)
    auto kernelEvents = GenKernelEventQueueWithTradComm();
    auto contextIdEvents = GenNodeCtxIdEventQueue();
    std::vector<std::pair<uint64_t, uint64_t>> items{
        {50, 2},     // Runtime OtherTask
        {110, 52},   // Runtime FFTS_PLUS
        {220, 1045}, // Runtime NOTIFY_RECORD
        {240, 1045}, // Runtime NOTIFY_RECORD
        {260, 0},    // Runtime KERNEL_AICORE
        {340, 1},    // Runtime KERNEL_AICPU
    };
    auto taskTrackEvents = GenTaskTrackEventQueue(items);

    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    cannWarehouse->kernelEvents = kernelEvents;
    cannWarehouse->taskTrackEvents = taskTrackEvents;
    cannWarehouse->contextIdEvents = contextIdEvents;

    // 建树
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);
    auto treeNode = treeBuilder->Build();

    // 分析树
    TypeData::GetInstance().Load("./");
    auto ana = std::make_shared<TreeAnalyzer>(treeNode, 1);
    return ana;
}

// 测试场景1 L0 : 静态图+计算类算子ffts+模式+hccl算子传统模式（MindSpore、Tensorflow、ACL推理）
TEST_F(TreeAnalyzerUTest, TestTreeAnalyzerWhenScenario1L0)
{
    auto ana = GetAnalyzerForScenario1L0();
    ana->Analyze();

    auto computeTasks = ana->GetComputeTasks();
    auto hcclTasks = ana->GetHCCLTasks();
    auto bigOPs = ana->GetHcclBigOps();
    auto tasks = ana->GetTasks();

    std::sort(computeTasks.begin(), computeTasks.end(), HostTaskCompCtx());

    std::vector<uint64_t> expectComputeTaskTimes{110, 110, 110, 110, 110, 110, 110, 110, 260, 340};
    std::vector<uint64_t> expectComputeTaskCtxIds{0, 1, 2, 3, 4, 5, 6, 7,
                                                  DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID};
    std::vector<uint64_t> expectHcclTaskTimes{220, 240, 260, 340};
    // 110中多一条是ffts+大任务的标记
    std::vector<uint64_t> expectTaskTimes{50, 110, 110, 110, 110, 110, 110, 110, 110, 110, 220, 240, 260, 340};
    // DEFAULT_CONTEXT_ID分别代表一个tk,
    std::vector<uint64_t> expectTaskCtxIds{0, 1, 2, 3, 4, 5, 6, 7, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID};
    // 特殊场景校验分别代表AI_CORE和HCCL_AICPU
    std::vector<uint64_t> expectHcclComputeTaskTaskType{9, 11};
    const uint64_t expectbigOPsNum = 3;
    // 先检查个数正确
    EXPECT_EQ(computeTasks.size(), expectComputeTaskTimes.size());
    EXPECT_EQ(hcclTasks.size(), expectHcclTaskTimes.size());
    EXPECT_EQ(bigOPs.size(), expectbigOPsNum);
    EXPECT_EQ(tasks.size(), expectTaskTimes.size());

    // 再匹配内容正确
    for (uint16_t i = 0; i < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i]->timeStamp, expectComputeTaskTimes[i]);
    }
    for (uint16_t i = 0; i < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i]->contextId, expectComputeTaskCtxIds[i]);
    }
    for (uint16_t i = 0; i < hcclTasks.size(); i++) {
        EXPECT_EQ(hcclTasks[i]->timeStamp, expectHcclTaskTimes[i]);
    }
    for (uint16_t i = 0; i < tasks.size(); i++) {
        EXPECT_EQ(tasks[i]->timeStamp, expectTaskTimes[i]);
    }

    std::sort(tasks.begin(), tasks.end(), HostTaskCompCtx());
    for (uint16_t i = 0; i < tasks.size(); i++) {
        EXPECT_EQ(tasks[i]->contextId, expectTaskCtxIds[i]);
    }

    uint32_t specialComputeTaskStart = 8;
    for (uint32_t i = 0; i + specialComputeTaskStart < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i + specialComputeTaskStart]->op->opDesc->nodeDesc->data.nodeBasicInfo.taskType,
                  expectHcclComputeTaskTaskType[i]);
    }
}

std::shared_ptr<TreeAnalyzer> GetAnalyzerForScenario1L1()
{
    // 逻辑树如下，其中c-n-t表示ctxId,nodeBasicInfo,tensorInfo，【d】表示异常情况，[n]标识每个context id event中带几个context id
    // ng表示图的nodeBasic数据
    // tk(260)    tk(340)为aicpu的hccl和aicore的hccl（reduce tbe）任务
    // Model :  [1,            200]  [201,                                    400]  [401, 800]
    // Node  :  [2, 99]  [100, 150]  [202,            250] [251,  301] [329, 351]
    //                  c-n-t(125)[2]        n(222)           n(255)     n(340)
    //                  c-n-t(126)[2]
    //                  c-n-t(127)[2]
    //                  c-n-t(128)[2]
    //                    ng(129)
    //                   t(144【d】)
    // Hccl  :                       [210, 220] [230, 250]  [252, 300] [330, 350]
    //                                hI(211)     hI(231)     hI(253)    hI(331)
    // Runtime:  tk(50)    tk(110)     tk(220)    tk(240)     tk(260)    tk(340)
    auto kernelEvents = GenKernelEventQueueWithTradComm();
    auto hcclInfoEvents = GenHcclInfoEventQueue({211, 231, 253, 331}, {DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID});
    auto nodeBasicInfoEvents = GenNodeBasicInfoEventQueue({{125, 1, 0}, {126, 2, 0}, {127, 1, 1},
                                                           {128, 0, 1}, {129, 100, 6}, {222, 6, 9},
                                                           {255, 7, 9}, {340, 8, 9}});
    auto tensorInfoEvents = GenTensorInfoEventQueue({125, 126, 127, 128, 144});
    auto contextIdEvents = GenNodeCtxIdEventQueue();
    std::vector<std::pair<uint64_t, uint64_t>> items{
        {50, 2},     // Runtime OtherTask
        {110, 52},   // Runtime FFTS_PLUS
        {220, 1045}, // Runtime NOTIFY_RECORD
        {240, 1045}, // Runtime NOTIFY_RECORD
        {260, 0},    // Runtime KERNEL_AICORE
        {340, 1},    // Runtime KERNEL_AICPU
    };
    auto taskTrackEvents = GenTaskTrackEventQueue(items);

    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    cannWarehouse->kernelEvents = kernelEvents;
    cannWarehouse->hcclInfoEvents = hcclInfoEvents;
    cannWarehouse->nodeBasicInfoEvents = nodeBasicInfoEvents;
    cannWarehouse->contextIdEvents = contextIdEvents;
    cannWarehouse->taskTrackEvents = taskTrackEvents;
    cannWarehouse->tensorInfoEvents = tensorInfoEvents;

    // 建树
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);
    auto treeNode = treeBuilder->Build();

    // 分析树
    TypeData::GetInstance().Load("./");
    auto ana = std::make_shared<TreeAnalyzer>(treeNode, 1);
    return ana;
}

// 测试场景1 L1 : 静态图+计算类算子ffts+模式+hccl算子传统模式（MindSpore、Tensorflow、ACL推理）
TEST_F(TreeAnalyzerUTest, TestTreeAnalyzerWhenScenario1L1)
{
    auto ana = GetAnalyzerForScenario1L1();
    ana->Analyze();

    auto computeTasks = ana->GetComputeTasks();
    auto hcclTasks = ana->GetHCCLTasks();
    auto bigOPs = ana->GetHcclBigOps();
    auto tasks = ana->GetTasks();

    std::sort(computeTasks.begin(), computeTasks.end(), HostTaskCompCtx());

    std::vector<uint64_t> expectComputeTaskTimes{110, 110, 110, 110, 110, 110, 110, 110, 110, 260, 340};
    std::vector<uint64_t> expectComputeTaskCtxIds{0, 1, 2, 3, 4, 5, 6, 7,
                                                  DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID};
    std::vector<uint64_t> expectHcclTaskTimes{220, 240, 260, 340};
    // 110中多一条是ffts+大任务的标记
    std::vector<uint64_t> expectTaskTimes{50, 110, 110, 110, 110, 110, 110, 110, 110, 110, 110, 220, 240, 260, 340};
    // DEFAULT_CONTEXT_ID分别代表一个tk,
    std::vector<uint64_t> expectTaskCtxIds{0, 1, 2, 3, 4, 5, 6, 7,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID};

    std::vector<uint64_t> expectComputeTaskOpTypes{1, 1, 2, 2, 1, 1, 0, 0, 0, 0};
    std::vector<uint64_t> expectComputeTaskTensorTimes{125, 125, 126, 126, 127, 127, 128, 128, 144};
    // 特殊场景校验分别代表AI_CORE和HCCL_AICPU
    std::vector<uint64_t> expectHcclComputeTaskTaskType{9, 11};
    const uint64_t expectbigOPsNum = 3;
    // 先检查个数正确
    EXPECT_EQ(computeTasks.size(), expectComputeTaskTimes.size());
    EXPECT_EQ(hcclTasks.size(), expectHcclTaskTimes.size());
    EXPECT_EQ(bigOPs.size(), expectbigOPsNum);
    EXPECT_EQ(tasks.size(), expectTaskTimes.size());

    // 再匹配内容正确
    for (uint16_t i = 0; i < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i]->timeStamp, expectComputeTaskTimes[i]);
    }

    for (uint16_t i = 0; i < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i]->contextId, expectComputeTaskCtxIds[i]);
    }

    uint32_t normalNum = 8;
    for (uint16_t i = 0; i < normalNum; i++) {
        EXPECT_EQ(computeTasks[i]->op->opDesc->nodeDesc->data.nodeBasicInfo.opType,
                  expectComputeTaskOpTypes[i]);
    }
    EXPECT_EQ(computeTasks[normalNum]->op, nullptr);

    for (uint16_t i = 0; i < normalNum; i++) {
        EXPECT_EQ(computeTasks[i]->op->opDesc->tensorDesc->timeStamp, expectComputeTaskTensorTimes[i]);
    }

    for (uint16_t i = 0; i < hcclTasks.size(); i++) {
        EXPECT_EQ(hcclTasks[i]->timeStamp, expectHcclTaskTimes[i]);
    }

    for (uint16_t i = 0; i < tasks.size(); i++) {
        EXPECT_EQ(tasks[i]->timeStamp, expectTaskTimes[i]);
    }

    std::sort(tasks.begin(), tasks.end(), HostTaskCompCtx());
    for (uint16_t i = 0; i < tasks.size(); i++) {
        EXPECT_EQ(tasks[i]->contextId, expectTaskCtxIds[i]);
    }

    uint32_t specialComputeTaskStart = 9;
    for (uint32_t i = 0; i + specialComputeTaskStart < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i + specialComputeTaskStart]->op->opDesc->nodeDesc->data.nodeBasicInfo.taskType,
                  expectHcclComputeTaskTaskType[i]);
    }
}

std::shared_ptr<TreeAnalyzer> GetAnalyzerForScenario2L0()
{
    // 逻辑树如下，其中[n]标识每个context id event中带几个context id
    // 其中tk(110),tk(120),tk(168)为计算任务
    // Model :  [1,                          200]  [201,             400]  [401, 800]
    // Node  :  [2, 99]  [100, 150]   [160, 195]   [202,  300] [302, 399]
    //                                 c(171)[1]
    // Hccl  :                                     [202,  300] [302, 399]
    //                                              c(230)[3]   c(310)[3]
    // Runtime:  tk(50)   tk(105)       tk(165)      tk(220)     tk(330)
    //                    tk(110)       tk(168)
    //                    tk(120)
    auto kernelEvents = GenKernelEventQueueWithFFtsComm();
    auto contextIdEvents = GenHcclCtxIdEventQueue();
    uint32_t nodeCtxDot = 171;
    FakeEventGenerator::AddNodeCtxIdEvent(contextIdEvents, nodeCtxDot, {0, 0});
    contextIdEvents->Sort();
    std::vector<std::pair<uint64_t, uint64_t>> items{
        {50, 2},   // Runtime OtherTask
        {105, 5},  // Runtime MEMCPY_ASYNC
        {110, 50}, // Runtime STARS_COMMON
        {120, 0},  // Runtime KERNEL_AICORE
        {165, 5},  // Runtime MEMCPY_ASYNC
        {168, 52}, // Runtime FFTS_PLUS
        {220, 52}, // Runtime FFTS_PLUS
        {330, 52}, // Runtime FFTS_PLUS
    };
    auto taskTrackEvents = GenTaskTrackEventQueue(items);

    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    cannWarehouse->kernelEvents = kernelEvents;
    cannWarehouse->contextIdEvents = contextIdEvents;
    cannWarehouse->taskTrackEvents = taskTrackEvents;

    // 建树
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);
    auto treeNode = treeBuilder->Build();

    // 分析树
    TypeData::GetInstance().Load("./");
    auto ana = std::make_shared<TreeAnalyzer>(treeNode, 1);
    return ana;
}

// 测试场景2 L0 : 计算类算子传统模式+hccl算子ffts+模式（Pytorch训练）
TEST_F(TreeAnalyzerUTest, TestTreeAnalyzerWhenScenario2L0)
{
    auto ana = GetAnalyzerForScenario2L0();
    ana->Analyze();

    auto computeTasks = ana->GetComputeTasks();
    auto hcclTasks = ana->GetHCCLTasks();
    auto bigOPs = ana->GetHcclBigOps();
    auto tasks = ana->GetTasks();

    std::vector<uint64_t> expectComputeTaskTimes{110, 120, 168};
    std::vector<uint64_t> expectComputeTaskCtxIds{DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, 0};
    std::vector<uint64_t> expectHcclTaskTimes{220, 220, 220, 330, 330, 330};
    // 多出的一个220，一个330分别标识ffts+大任务, 多出的一个168标识MIX算子
    std::vector<uint64_t> expectTaskTimes{50, 105, 110, 120, 165, 168, 168, 220, 220, 220, 220, 330, 330, 330, 330};
    // DEFAULT_CONTEXT_ID分别代表一个tk,
    std::vector<uint64_t> expectTaskCtxIds{0, 0, 0, 1, 1, 2, 2,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID};
    const uint64_t expectbigOPsNum = 2;
    // 先检查个数正确
    EXPECT_EQ(computeTasks.size(), expectComputeTaskTimes.size());
    EXPECT_EQ(hcclTasks.size(), expectHcclTaskTimes.size());
    EXPECT_EQ(bigOPs.size(), expectbigOPsNum);
    EXPECT_EQ(tasks.size(), expectTaskTimes.size());

    // 再匹配内容正确
    for (uint16_t i = 0; i < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i]->timeStamp, expectComputeTaskTimes[i]);
    }
    for (uint16_t i = 0; i < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i]->contextId, expectComputeTaskCtxIds[i]);
    }
    for (uint16_t i = 0; i < hcclTasks.size(); i++) {
        EXPECT_EQ(hcclTasks[i]->timeStamp, expectHcclTaskTimes[i]);
    }
    for (uint16_t i = 0; i < tasks.size(); i++) {
        EXPECT_EQ(tasks[i]->timeStamp, expectTaskTimes[i]);
    }

    std::sort(tasks.begin(), tasks.end(), HostTaskCompCtx());
    for (uint16_t i = 0; i < tasks.size(); i++) {
        EXPECT_EQ(tasks[i]->contextId, expectTaskCtxIds[i]);
    }
}

std::shared_ptr<TreeAnalyzer> GetAnalyzerForScenario2L1()
{
    // 逻辑树如下，其中[n]标识每个event中带几个context id
    // 其中tk(110),tk(120),tk(168)为计算任务
    // Model :  [1,                          200]  [201,              400]  [401, 800]
    // Node  :  [2, 99]  [100, 150]   [160, 195]   [202,  300] [302,  399]
    //                    n-t(125)     c-n-t(171)    n(250)       n(310)
    // Hccl  :                                     [202,  300] [302,  399]
    //                                               c(230)[3]  c(310)[3]
    //                                            hInfo(230)[3] hInfo(310)[3]
    // Runtime:  tk(50)   tk(105)       tk(165)       tk(220)     tk(330)
    //                    tk(110)       tk(168)
    //                    tk(120)
    auto kernelEvents = GenKernelEventQueueWithFFtsComm();
    auto hcclInfoEvents = GenHcclInfoEventQueue({230, 230, 230, 310, 310, 310}, {0, 1, 2, 0, 1, 2});
    auto nodeBasicInfoEvents = GenNodeBasicInfoEventQueue({{125, 1, 1}, {171, 3, 2}, {250, 333, 9}, {310, 334, 9}});
    auto tensorInfoEvents = GenTensorInfoEventQueue({125, 171});
    auto contextIdEvents = GenHcclCtxIdEventQueue();
    uint32_t nodeCtxDot = 171;
    FakeEventGenerator::AddNodeCtxIdEvent(contextIdEvents, nodeCtxDot, {0, 0});
    contextIdEvents->Sort();
    std::vector<std::pair<uint64_t, uint64_t>> items{
        {50, 2},   // Runtime OtherTask
        {105, 5},  // Runtime MEMCPY_ASYNC
        {110, 50}, // Runtime STARS_COMMON
        {120, 0},  // Runtime KERNEL_AICORE
        {165, 5},  // Runtime MEMCPY_ASYNC
        {168, 52}, // Runtime FFTS_PLUS
        {220, 52}, // Runtime FFTS_PLUS
        {330, 52}, // Runtime FFTS_PLUS
    };
    auto taskTrackEvents = GenTaskTrackEventQueue(items);

    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    cannWarehouse->kernelEvents = kernelEvents;
    cannWarehouse->hcclInfoEvents = hcclInfoEvents;
    cannWarehouse->nodeBasicInfoEvents = nodeBasicInfoEvents;
    cannWarehouse->contextIdEvents = contextIdEvents;
    cannWarehouse->taskTrackEvents = taskTrackEvents;
    cannWarehouse->tensorInfoEvents = tensorInfoEvents;

    // 建树
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);
    auto treeNode = treeBuilder->Build();

    // 分析树
    TypeData::GetInstance().Load("./");
    auto ana = std::make_shared<TreeAnalyzer>(treeNode, 1);
    return ana;
}

// 测试场景2 L1 : 计算类算子传统模式+hccl算子ffts+模式（Pytorch训练）
TEST_F(TreeAnalyzerUTest, TestTreeAnalyzerWhenScenario2L1)
{
    auto ana = GetAnalyzerForScenario2L1();
    ana->Analyze();

    auto computeTasks = ana->GetComputeTasks();
    auto hcclTasks = ana->GetHCCLTasks();
    auto bigOPs = ana->GetHcclBigOps();
    auto tasks = ana->GetTasks();

    std::vector<uint64_t> expectComputeTaskTimes{110, 120, 168};
    std::vector<uint64_t> expectComputeTaskCtxIds{0, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID};
    std::vector<uint64_t> expectHcclTaskTimes{220, 220, 220, 330, 330, 330};
    std::vector<uint64_t> expectHcclTaskCtxIds{0, 0, 1, 1, 2, 2};
    // 多出的一个220，一个330分别标识ffts+大任务, 多出的一个168标识MIX算子
    std::vector<uint64_t> expectTaskTimes{50, 105, 110, 120, 165, 168, 168, 220, 220, 220, 220, 330, 330, 330, 330};
    // DEFAULT_CONTEXT_ID分别代表一个tk,
    std::vector<uint64_t> expectTaskCtxIds{0, 0, 0, 1, 1, 2, 2,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID,
                                           DEFAULT_CONTEXT_ID, DEFAULT_CONTEXT_ID};
    const uint64_t expectbigOPsNum = 2;

    std::vector<uint64_t> expectComputeTaskOpTypes{1, 1, 3};
    std::vector<uint64_t> expectComputeTaskTensorTimes{125, 125, 171};

    // 先检查个数正确
    EXPECT_EQ(computeTasks.size(), expectComputeTaskTimes.size());
    EXPECT_EQ(hcclTasks.size(), expectHcclTaskTimes.size());
    EXPECT_EQ(bigOPs.size(), expectbigOPsNum);
    EXPECT_EQ(tasks.size(), expectTaskTimes.size());

    // 再匹配内容正确
    for (uint16_t i = 0; i < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i]->timeStamp, expectComputeTaskTimes[i]);
    }

    for (uint16_t i = 0; i < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i]->op->opDesc->nodeDesc->data.nodeBasicInfo.opType,
                  expectComputeTaskOpTypes[i]);
    }

    for (uint16_t i = 0; i < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i]->op->opDesc->tensorDesc->timeStamp, expectComputeTaskTensorTimes[i]);
    }

    for (uint16_t i = 0; i < hcclTasks.size(); i++) {
        EXPECT_EQ(hcclTasks[i]->timeStamp, expectHcclTaskTimes[i]);
    }

    for (uint16_t i = 0; i < tasks.size(); i++) {
        EXPECT_EQ(tasks[i]->timeStamp, expectTaskTimes[i]);
    }

    std::sort(tasks.begin(), tasks.end(), HostTaskCompCtx());
    for (uint16_t i = 0; i < tasks.size(); i++) {
        EXPECT_EQ(tasks[i]->contextId, expectTaskCtxIds[i]);
    }

    // 由于unordered_map的特性，context_id不保序，先排序
    std::sort(computeTasks.begin(), computeTasks.end(), HostTaskCompCtx());
    for (uint16_t i = 0; i < computeTasks.size(); i++) {
        EXPECT_EQ(computeTasks[i]->contextId, expectComputeTaskCtxIds[i]);
    }

    // 由于unordered_map的特性，context_id不保序，先排序
    std::sort(hcclTasks.begin(), hcclTasks.end(), HostTaskCompCtx());
    for (uint16_t i = 0; i < hcclTasks.size(); i++) {
        EXPECT_EQ(hcclTasks[i]->contextId, expectHcclTaskCtxIds[i]);
    }
}