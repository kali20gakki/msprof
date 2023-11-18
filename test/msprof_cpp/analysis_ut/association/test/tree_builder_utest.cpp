/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : tree_builder_utest.cpp
 * Description        : TreeBuilder UT
 * Author             : msprof team
 * Creation Date      : 2023/11/13
 * *****************************************************************************
 */
#include <string>
#include <memory>
#include <vector>
#include <queue>
#include "gtest/gtest.h"
#include "event.h"
#include "tree.h"
#include "cann_warehouse.h"
#include "tree_builder.h"

using namespace Analysis::Parser::Host::Cann;
using namespace Analysis::Association::Cann;
using namespace Analysis::Entities;

class TreeBuilderUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

void AddApiEvent(std::shared_ptr<EventQueue> &eventQueue,
                 uint16_t level,
                 uint64_t start,
                 uint64_t end, const std::string &desc)
{
    EventInfo testInfo{EventType::EVENT_TYPE_API, level, start, end};
    auto eventPtr = std::make_shared<Event>(nullptr, desc, testInfo);
    eventQueue->Push(eventPtr);
}

void AddTaskTrackEvent(std::shared_ptr<EventQueue> &eventQueue,
                       uint64_t dot, const std::string &desc)
{
    EventInfo testInfo{EventType::EVENT_TYPE_TASK_TRACK, EVENT_LEVEL_RUNTIME, dot, dot};
    auto eventPtr = std::make_shared<Event>(nullptr, desc, testInfo);
    eventQueue->Push(eventPtr);
}

void AddNodeBasicEvent(std::shared_ptr<EventQueue> &eventQueue,
                       uint64_t dot, const std::string &desc)
{
    EventInfo testInfo{EventType::EVENT_TYPE_NODE_BASIC_INFO, EVENT_LEVEL_NODE, dot, dot};
    auto eventPtr = std::make_shared<Event>(nullptr, desc, testInfo);
    eventQueue->Push(eventPtr);
}

void AddTensorInfoEvent(std::shared_ptr<EventQueue> &eventQueue,
                        uint64_t dot, const std::string &desc)
{
    EventInfo testInfo{EventType::EVENT_TYPE_TENSOR_INFO, EVENT_LEVEL_NODE, dot, dot};
    auto eventPtr = std::make_shared<Event>(nullptr, desc, testInfo);
    eventQueue->Push(eventPtr);
}

void AddCtxIdEvent(std::shared_ptr<EventQueue> &eventQueue,
                   uint64_t dot, uint16_t level, const std::string &desc)
{
    EventInfo testInfo{EventType::EVENT_TYPE_CONTEXT_ID, level, dot, dot};
    auto eventPtr = std::make_shared<Event>(nullptr, desc, testInfo);
    eventQueue->Push(eventPtr);
}

void AddGraphIdMapEvent(std::shared_ptr<EventQueue> &eventQueue,
                        uint64_t dot, const std::string &desc)
{
    EventInfo testInfo{EventType::EVENT_TYPE_GRAPH_ID_MAP, EVENT_LEVEL_MODEL, dot, dot};
    auto eventPtr = std::make_shared<Event>(nullptr, desc, testInfo);
    eventQueue->Push(eventPtr);
}

void AddFusionOpInfoEvent(std::shared_ptr<EventQueue> &eventQueue,
                          uint64_t dot, const std::string &desc)
{
    EventInfo testInfo{EventType::EVENT_TYPE_FUSION_OP_INFO, EVENT_LEVEL_MODEL, dot, dot};
    auto eventPtr = std::make_shared<Event>(nullptr, desc, testInfo);
    eventQueue->Push(eventPtr);
}

void AddHcclInfoEvent(std::shared_ptr<EventQueue> &eventQueue,
                      uint64_t dot, const std::string &desc)
{
    EventInfo testInfo{EventType::EVENT_TYPE_HCCL_INFO, EVENT_LEVEL_HCCL, dot, dot};
    auto eventPtr = std::make_shared<Event>(nullptr, desc, testInfo);
    eventQueue->Push(eventPtr);
}

std::shared_ptr<EventQueue> GenNodeBasicInfoEvents()
{
    auto nodeBasicInfoEvents = std::make_shared<EventQueue>(1, 10);
    std::unordered_map<std::string, std::vector<uint64_t>> Events{
        // NodeLevel: {"Node", {{110, 130}, {150, 170}}},
        //  Index      0     1    2    3    4    5   6     7    8    9    10
        {"Node", {105, 110, 115, 115, 130, 135, 150, 155, 160, 170, 1000}}
    };
    int cnt = 0;
    for (auto dot: Events["Node"]) {
        AddNodeBasicEvent(nodeBasicInfoEvents, dot, "NodeBasicInfo" + std::to_string(cnt));
        cnt++;
    }
    nodeBasicInfoEvents->Sort();
    return nodeBasicInfoEvents;
}

std::shared_ptr<EventQueue> GenTensorInfoEvents()
{
    auto tensorInfoEvents = std::make_shared<EventQueue>(1, 10);
    std::unordered_map<std::string, std::vector<uint64_t>> Events{
        // NodeLevel: {"Node", {{110, 130}, {150, 170}}},
        //  Index      0     1    2    3    4    5   6     7    8    9
        {"Node", {109, 110, 113, 130, 134, 150, 151, 152, 170, 175}}
    };
    int cnt = 0;
    for (auto dot: Events["Node"]) {
        AddTensorInfoEvent(tensorInfoEvents, dot, "TensorInfo" + std::to_string(cnt));
        cnt++;
    }
    tensorInfoEvents->Sort();
    return tensorInfoEvents;
}

std::shared_ptr<EventQueue> GenCtxIdEvents()
{
    auto contextIdEvents = std::make_shared<EventQueue>(1, 20);
    std::unordered_map<std::string, std::vector<uint64_t>> nodelLevelEvents{
        // NodeLevel: {"Node", {{110, 130}, {150, 170}}},
        //  Index      0     1    2    3    4    5   6     7
        {"Node", {109, 110, 130, 134, 150, 151, 170, 172}}
    };

    std::unordered_map<std::string, std::vector<uint64_t>> hcclLevelEvents{
        // HcclLevel: {"Hccl", {{115, 125}, {155, 165}}},
        //  Index      0     1    2    3    4
        {"Hccl", {115, 117, 125, 130, 160}}
    };

    int nodeCnt = 0;
    for (auto dot: nodelLevelEvents["Node"]) {
        AddCtxIdEvent(contextIdEvents, dot, EVENT_LEVEL_NODE, "CtxId" + std::to_string(nodeCnt));
        nodeCnt++;
    }

    int hcclCnt = 0;
    for (auto dot: hcclLevelEvents["Hccl"]) {
        AddCtxIdEvent(contextIdEvents, dot, EVENT_LEVEL_HCCL, "CtxId" + std::to_string(hcclCnt));
        hcclCnt++;
    }
    contextIdEvents->Sort();
    return contextIdEvents;
}

std::shared_ptr<EventQueue> GenGraphIdMapEvents()
{
    auto graphIdMapEvents = std::make_shared<EventQueue>(1, 10);
    std::unordered_map<std::string, std::vector<uint64_t>> Events{
        //  Model   {"Model", {{110, 200}}},
        //  Index      0     1    2    3    4
        {"Model", {105, 110, 115, 200, 210}}
    };
    int cnt = 0;
    for (auto dot: Events["Model"]) {
        AddGraphIdMapEvent(graphIdMapEvents, dot, "GraphIdMap" + std::to_string(cnt));
        cnt++;
    }
    graphIdMapEvents->Sort();
    return graphIdMapEvents;
}

std::shared_ptr<EventQueue> GenFusionOpInfoEvents()
{
    auto fusioOpInfoEvents = std::make_shared<EventQueue>(1, 10);
    std::unordered_map<std::string, std::vector<uint64_t>> Events{
        //  Model   {"Model", {{110, 200}}},
        //  Index       0     1    2    3    4
        {"Model", {104, 110, 116, 200, 220}}
    };
    int cnt = 0;
    for (auto dot: Events["Model"]) {
        AddFusionOpInfoEvent(fusioOpInfoEvents, dot, "FusionOpInfo" + std::to_string(cnt));
        cnt++;
    }
    fusioOpInfoEvents->Sort();
    return fusioOpInfoEvents;
}

std::shared_ptr<EventQueue> GenHcclInfoEvents()
{
    auto hcclInfoEvents = std::make_shared<EventQueue>(1, 10);
    std::unordered_map<std::string, std::vector<uint64_t>> Events{
        // HcclLevel: {"Hccl", {{115, 125}, {155, 165}}},
        //  Index       0     1    2    3    4   5     6    7    8   9
        {"Hccl", {103, 110, 115, 120, 125, 130, 155, 160, 165, 170}}
    };
    int cnt = 0;
    for (auto dot: Events["Hccl"]) {
        AddHcclInfoEvent(hcclInfoEvents, dot, "HcclInfo" + std::to_string(cnt));
        cnt++;
    }
    hcclInfoEvents->Sort();
    return hcclInfoEvents;
}
/*
 EventLevel      此Level包含的EventType
|- ACL
|- Model    [graph_id_map, fusion_op_info]
|- Node     [node_basic_info, tensor_info, context_id]
|- HCCL     [hccl_info, context_id]
|- Runtime  [task_track, mem_cpy]
*/
std::shared_ptr<EventQueue> GenKernelEvents()
{
    auto kernelEvents = std::make_shared<EventQueue>(1, 100);
    std::vector<std::string> levels{"Model", "Node", "Hccl"};
    std::unordered_map<std::string, uint16_t> levelNum{
        {"Model", EVENT_LEVEL_MODEL},
        {"Node", EVENT_LEVEL_NODE},
        {"Hccl", EVENT_LEVEL_HCCL},
    };
    std::unordered_map<std::string, std::vector<std::pair<uint64_t, uint64_t>>> apiEvents{
        {"Model", {{110, 200}}},
        {"Node", {{110, 130}, {150, 170}, {175, 180}}},
        {"Hccl", {{115, 125}, {155, 165}}},
    };

    for (const auto& le: levels) {
        int cnt = 0;
        for (auto range: apiEvents[le]) {
            AddApiEvent(kernelEvents, levelNum[le], range.first, range.second, le + std::to_string(cnt));
            cnt++;
        }
    }
    kernelEvents->Sort();
    return kernelEvents;
}

std::shared_ptr<EventQueue> GenTaskTrackEvents()
{
    auto taskTrackEvents = std::make_shared<EventQueue>(1, 100);
    std::unordered_map<std::string, std::vector<uint64_t>> ttEvents{
        /*    Index        0    1    2    3    4    5    6    7    8 */
        {"Runtime", {114, 115, 120, 125, 130, 155, 160, 165, 170, 175, 180, 190}},
    };
    int cnt = 0;
    for (auto dot: ttEvents["Runtime"]) {
        AddTaskTrackEvent(taskTrackEvents, dot, "TaskTrack" + std::to_string(cnt));
        cnt++;
    }
    taskTrackEvents->Sort();
    return taskTrackEvents;
}


TEST_F(TreeBuilderUTest, TestAddLevelEventsShouldReturnFalseWhenInputNullptrOrEmpty)
{
    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);

    /* 测试events为nullptr，levelNodes不为空的情况 */
    std::vector<std::shared_ptr<TreeNode>> oneNodes{std::shared_ptr<TreeNode>{}};
    auto nullEventQueuePtr = std::shared_ptr<EventQueue>{};
    auto checkNullPtr = treeBuilder->AddLevelEvents(nullEventQueuePtr, oneNodes);
    EXPECT_EQ(false, checkNullPtr);

    /* 测试events不为nullptr，levelNodes为空的情况 */
    auto eventQueuePtr = std::make_shared<EventQueue>(1, 5);
    std::vector<std::shared_ptr<TreeNode>> emptyNodes{};
    auto checkEmptyNodes = treeBuilder->AddLevelEvents(eventQueuePtr, emptyNodes);
    EXPECT_EQ(false, checkEmptyNodes);

    /* 测试events为nullptr，levelNodes为空的情况 */
    auto checkAllEmpty = treeBuilder->AddLevelEvents(nullEventQueuePtr, emptyNodes);
    EXPECT_EQ(false, checkEmptyNodes);
}

TEST_F(TreeBuilderUTest, TestAddLevelEventsShouldMatchCorrectNodeWhenInputNotEmpty)
{
    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);

    /* 构造数据：TreeNodes：     2---3        5---------8    10-----12          */
    /* 构造数据：Events：   1 1  2       4 4     6 6  7 8  9     11 12 13 13    */
    std::vector<std::shared_ptr<TreeNode>> levelNodes{};
    std::vector<std::pair<uint64_t, uint64_t>> nodesTimes{{2, 3}, {5, 8}, {10, 12}};
    for (auto range: nodesTimes) {
        EventInfo info{EventType::EVENT_TYPE_API, 0, range.first, range.second};
        auto event = std::make_shared<Event>(nullptr, "test", info);
        levelNodes.emplace_back(std::make_shared<TreeNode>(event));
    }

    std::vector<uint64_t> eventDots{1, 1, 2, 4, 4, 6, 6, 7, 8, 9, 11, 12, 12, 13, 14};
    std::vector<uint64_t> ans{6, 6, 7, 8, 11, 12, 12};
    auto eventQueue = std::make_shared<EventQueue>(1, eventDots.size());
    for (auto dot: eventDots) {
        EventInfo info{EventType::EVENT_TYPE_NODE_BASIC_INFO, 0, dot, dot};
        auto eventPtr = std::make_shared<Event>(nullptr, "test", info);
        eventQueue->Push(eventPtr);
    }

    auto checkFalse = treeBuilder->AddLevelEvents(eventQueue, levelNodes);
    /* 因为存在没有匹配上的元素，范围值应为false */
    EXPECT_EQ(false, checkFalse);

    std::vector<uint64_t> vaild;
    for (auto node: levelNodes) {
        if (!node->records.empty()) {
            for (const auto &e: node->records) {
                vaild.emplace_back(e->info.start);
            }
        }
    }

    /* 首先需要保证size相等，然后检查每个元素是否相等 */
    EXPECT_EQ(ans.size(), vaild.size());
    for (int i = 0; i < vaild.size(); ++i) {
        EXPECT_EQ(ans[i], vaild[i]);
    }
}

TEST_F(TreeBuilderUTest, TestBuildTreeShouldReturnRootNodeWhenCANNWarehouseEmpty)
{
    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    /* 建树 */
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);
    auto tree = treeBuilder->Build();
    EXPECT_EQ(nullptr, tree);
}

TEST_F(TreeBuilderUTest, TestBuildTreeShouldBuildCorrectTreeWhenCANNWarehouseNotEmpty)
{
    /* 生成cannWarehouse各个type的数据 */
    auto kernelEvents = GenKernelEvents();
    auto nodeBasicInfoEvents = GenNodeBasicInfoEvents();
    auto tensorInfoEvents = GenTensorInfoEvents();
    auto contextIdEvents = GenCtxIdEvents();
    auto graphIdMapEvents = GenGraphIdMapEvents();
    auto fusionOpInfoEvents = GenFusionOpInfoEvents();
    auto hcclInfoEvents = GenHcclInfoEvents();
    auto taskTrackEvents = GenTaskTrackEvents();

    auto cannWarehouse = std::make_shared<CANNWarehouse>();
    cannWarehouse->kernelEvents = kernelEvents;
    cannWarehouse->nodeBasicInfoEvents = nodeBasicInfoEvents;
    cannWarehouse->tensorInfoEvents = tensorInfoEvents;
    cannWarehouse->contextIdEvents = contextIdEvents;
    cannWarehouse->graphIdMapEvents = graphIdMapEvents;
    cannWarehouse->fusionOpInfoEvents = fusionOpInfoEvents;
    cannWarehouse->hcclInfoEvents = hcclInfoEvents;
    cannWarehouse->taskTrackEvents = taskTrackEvents;

    /* 期望生成的Tree每一层的字符串 */
    std::vector<std::string> ans{
        "RootEvent ", // Root Level
        "Model0 [GraphIdMap2] [GraphIdMap3] [FusionOpInfo2] [FusionOpInfo3] "  // Model Level
        "VirtualEvent [TaskTrack0] VirtualEvent [TaskTrack1] VirtualEvent [TaskTrack4] "
        "VirtualEvent [TaskTrack5] VirtualEvent [TaskTrack8] VirtualEvent [TaskTrack9] ",
        "Node0 [NodeBasicInfo2] [NodeBasicInfo3] [NodeBasicInfo4] [TensorInfo2] [TensorInfo3] " // Node Level
        "[CtxId2] Node1 [NodeBasicInfo7] [NodeBasicInfo8] [NodeBasicInfo9] [TensorInfo6] "
        "[TensorInfo7] [TensorInfo8] [CtxId5] [CtxId6] Node2 ",
        "Hccl0 [CtxId1] [CtxId2] [HcclInfo3] [HcclInfo4] Hccl1 [CtxId4] [HcclInfo7] " // HCCL Level
        "[HcclInfo8] VirtualEvent [TaskTrack10] ",
        "VirtualEvent [TaskTrack2] VirtualEvent [TaskTrack3] VirtualEvent [TaskTrack6] " // Virtual(Runtime) Level
        "VirtualEvent [TaskTrack7] ",
    };

    /* 建树 */
    auto treeBuilder = std::make_shared<TreeBuilder>(cannWarehouse, 1);
    auto treeNode = treeBuilder->Build();
    auto tree = std::make_shared<Tree>(treeNode);
    /* 层序遍历建完的树，将树的每一层保存为一个字符串 */
    auto levelStr = tree->Show();

    /* 首先需要保证size相等，然后检查每个元素是否相等 */
    EXPECT_EQ(ans.size(), levelStr.size());
    for (int i = 0; i < ans.size(); ++i) {
        EXPECT_EQ(ans[i], levelStr[i]);
    }
}