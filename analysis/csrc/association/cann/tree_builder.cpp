/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : tree_builder.cpp
 * Description        : EventTree建树模块：遍历EventQueue建树
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#include "analysis/csrc/association/cann/tree_builder.h"
#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/entities/event.h"
#include "analysis/csrc/utils/utils.h"

namespace Analysis {
namespace Association {
namespace Cann {

namespace {
const int MAX_DEPTH = 20;
}

using namespace Analysis::Utils;
/*
 EventLevel      此Level包含的EventType
|- ACL
|- Model    [graph_id_map, fusion_op_info]
|- Node     [node_basic_info, tensor_info, context_id]
|- HCCL     [hccl_info, context_id]
|- Runtime  [task_track, mem_cpy]

建树对外接口：
1. 使用Model、Node、HCCL层的Api Event建立核心树
    建树过程中需要记录 a.各个层级的api  b.叶子节点

2. 向核心树的各个level的节点上挂附加信息event
3. 向核心树的叶子节点上添加tacktrack event
*/
std::shared_ptr<TreeNode> TreeBuilder::Build()
{
    if (!cannWarehouse_->kernelEvents && !cannWarehouse_->taskTrackEvents) {
        WARN("No key profiling info to build tree, threadId = %", threadId_);
        return nullptr;
    }
    kernelEvents_ = cannWarehouse_->kernelEvents;
    auto graphIdMapEvents = cannWarehouse_->graphIdMapEvents;
    auto fusionOpInfoEvents = cannWarehouse_->fusionOpInfoEvents;
    auto nodeBasicInfoEvents = cannWarehouse_->nodeBasicInfoEvents;
    auto tensorInfoEvents = cannWarehouse_->tensorInfoEvents;
    auto contextIdEvents = cannWarehouse_->contextIdEvents;
    auto hcclInfoEvents = cannWarehouse_->hcclInfoEvents;
    auto taskTrackEvents = cannWarehouse_->taskTrackEvents;

    auto rootNode = GenerateRoot();
    std::shared_ptr<TreeNode> tree;
    if (kernelEvents_ and !kernelEvents_->Empty()) {
        // 1. 用Api和TaskTrack Event建立核心树
        tree = BuildTree(rootNode, 0);
    } else {
        // 1. root节点就是整颗核心树
        tree = rootNode;
        leafNodes_.emplace_back(rootNode);
    }
    auto ctxIdPair = GroupCtxIdEvents(contextIdEvents);
    auto nodeCtxIdEvents = ctxIdPair.first;
    auto hcclCtxIdEvents = ctxIdPair.second;

    // 2. 向核心树的对应Level的TreeNode添加附加Event
    // 多线程添加，每一个Level一个线程
    const uint16_t levels = 3;
    ThreadPool pool(levels);
    pool.Start();

    // Model Level
    pool.AddTask([this, &graphIdMapEvents, &fusionOpInfoEvents]() {
        AddLevelEvents(graphIdMapEvents, modelLevelNodes_, EventType::EVENT_TYPE_GRAPH_ID_MAP);
        AddLevelEvents(fusionOpInfoEvents, modelLevelNodes_, EventType::EVENT_TYPE_FUSION_OP_INFO);
    });

    // Node Level
    pool.AddTask([this, &nodeCtxIdEvents, &tensorInfoEvents, &nodeBasicInfoEvents]() {
        AddLevelEvents(nodeBasicInfoEvents, nodeLevelNodes_, EventType::EVENT_TYPE_NODE_BASIC_INFO);
        AddLevelEvents(tensorInfoEvents, nodeLevelNodes_, EventType::EVENT_TYPE_TENSOR_INFO);
        AddLevelEvents(nodeCtxIdEvents, nodeLevelNodes_, EventType::EVENT_TYPE_CONTEXT_ID);
    });

    // Hccl level
    pool.AddTask([this, &hcclCtxIdEvents, &hcclInfoEvents]() {
        AddLevelEvents(hcclCtxIdEvents, hcclLevelNodes_, EventType::EVENT_TYPE_CONTEXT_ID);
        AddLevelEvents(hcclInfoEvents, hcclLevelNodes_, EventType::EVENT_TYPE_HCCL_INFO);
    });

    pool.WaitAllTasks();
    pool.Stop();
    // 向叶子节点添加TaskTrack类型的events
    AddTaskTrackEvents(rootNode, taskTrackEvents, leafNodes_);
    AddMissedDummyEvents(rootNode, modelLevelNodes_, missDummyNodes_);
    INFO("Build Tree End, ThreadId = %", threadId_);
    return tree;
}

std::shared_ptr<TreeNode> TreeBuilder::GenerateRoot()
{
    uint64_t bound;
    // rootNode的时间片应该覆盖整个EventQueue
    if (!kernelEvents_ or kernelEvents_->Empty()) {
        bound = cannWarehouse_->taskTrackEvents->GetBound();
    } else {
        bound = kernelEvents_->GetBound();
    }
    EventInfo rootInfo{EventType::EVENT_TYPE_API, 0, 0, bound + 1};
    std::shared_ptr<MsprofApi> api;
    MAKE_SHARED0_RETURN_VALUE(api, MsprofApi, nullptr);
    std::shared_ptr<Event> rootEvent;
    MAKE_SHARED_RETURN_VALUE(rootEvent, Event, nullptr, api, rootInfo);
    std::shared_ptr<TreeNode> rootNode;
    MAKE_SHARED_RETURN_VALUE(rootNode, TreeNode, nullptr, rootEvent);
    return rootNode;
}

// 将ctxIdEvents按Level分为Node和HCCL层
EventQueuePair TreeBuilder::GroupCtxIdEvents(std::shared_ptr<EventQueue> &ctxIdEvents)
{
    uint64_t ctxIdSize = 0;
    if (ctxIdEvents) {
        ctxIdSize = ctxIdEvents->GetSize();
    }
    std::shared_ptr<EventQueue> nodeCtxIdEvent;
    MAKE_SHARED_RETURN_VALUE(nodeCtxIdEvent, EventQueue, {}, threadId_, ctxIdSize);
    std::shared_ptr<EventQueue> hcclCtxIdEvent;
    MAKE_SHARED_RETURN_VALUE(hcclCtxIdEvent, EventQueue, {}, threadId_, ctxIdSize);

    while (ctxIdEvents && !ctxIdEvents->Empty()) {
        auto ctxId = ctxIdEvents->Pop();
        if (ctxId->info.level == MSPROF_REPORT_NODE_LEVEL) {
            nodeCtxIdEvent->Push(ctxId);
        } else if (ctxId->info.level == MSPROF_REPORT_HCCL_NODE_LEVEL) {
            hcclCtxIdEvent->Push(ctxId);
        }
    }

    return std::make_pair(nodeCtxIdEvent, hcclCtxIdEvent);
}

// 双指针遍历向Model Node HCCL Level的TreeNode中添加events
bool TreeBuilder::AddLevelEvents(std::shared_ptr<EventQueue> &events,
                                 std::vector<std::shared_ptr<TreeNode>> &levelNodes,
                                 EventType eventType) const
{
    // 检查输入各Type的Events指针
    if (!events || events->Empty()) {
        WARN("No Events, threadId = %, event type: %", threadId_, static_cast<int>(eventType));
        return false;
    }
    // 保证非空
    if (levelNodes.empty()) {
        ERROR("LevelNodes is empty, threadId = % ", threadId_);
        return false;
    }
    uint64_t matchCnt = 0; // 在时间片范围内的数量
    uint64_t mismatchCnt = 0; // 在时间片范围外的数量
    auto it = levelNodes.begin();
    while (!events->Empty() && it != levelNodes.end()) {
        auto event = events->Top();
        if (event->info.start > (*it)->event->info.start && event->info.start <= (*it)->event->info.end) {
            // event在TreeNode时间片内：(start, end]，将event记录在TreeNode的附加信息中
            (*it)->records.emplace_back(event);
            events->Pop();
            matchCnt++;
        } else if (event->info.start > (*it)->event->info.end) {
            // event超过TreeNode的结束时间，需要检查是否在下一个TreeNode的开始之前
            if (std::next(it) != levelNodes.end() && event->info.start < (*std::next(it))->event->info.start) {
                events->Pop(); // 丢掉此event
                mismatchCnt++;
                ERROR("Drop event threadId = %, start = %, event type: %, its start time between "
                      "last TreeNode end and next TreeNode start", threadId_, event->info.start,
                      static_cast<int>(eventType));
            }
            it++;
        } else {
            // event小于TreeNode的开始时间，此event对应的TreeNode丢失
            events->Pop(); // 丢掉此event
            mismatchCnt++;
            ERROR("Drop event threadId = %, event type: %, event time range: [%, %], node time range: [%, %]",
                  threadId_, static_cast<int>(eventType), event->info.start, event->info.end,
                  (*it)->event->info.start, (*it)->event->info.end);
        }
    }
    INFO("Add LevelEvents done, threadId = %, event type: %, matchCnt = %, mismatchCnt = % ",
         threadId_, static_cast<int>(eventType), matchCnt, mismatchCnt);
    // events没有匹配完
    if (!events->Empty()) {
        ERROR("After matching events is not empty, size = %, threadId = %", events->GetSize(), threadId_);
        return false;
    }
    return true;
}

// 双指针遍历向核心树叶子中添加TaskTrackEvents
bool TreeBuilder::AddTaskTrackEvents(std::shared_ptr<TreeNode> &rootNode,
                                     std::shared_ptr<EventQueue> &events,
                                     std::vector<std::shared_ptr<TreeNode>> &leafNodes)
{
    // 检查输入Runtime Events指针
    if (!events || events->Empty()) {
        ERROR("TaskTrack Events is empty, threadId = %", threadId_);
        return false;
    }
    // 保证非空
    if (leafNodes.empty()) {
        ERROR("LevelNodes is empty, threadId = % ", threadId_);
        return false;
    }
    uint64_t matchCnt = 0; // 在时间片范围内的数量
    uint64_t mismatchCnt = 0; // 在时间片范围外的数量
    auto it = leafNodes.begin();
    while (!events->Empty() && it != leafNodes.end()) {
        auto event = events->Top();
        // 建立dummy节点
        EventInfo dummyInfo{EventType::EVENT_TYPE_DUMMY, MSPROF_REPORT_RUNTIME_LEVEL,
                            event->info.start, event->info.end};
        std::shared_ptr<MsprofApi> api;
        MAKE_SHARED0_RETURN_VALUE(api, MsprofApi, false);
        std::shared_ptr<Event> dummyEvent;
        MAKE_SHARED_RETURN_VALUE(dummyEvent, Event, false, api, dummyInfo);
        std::shared_ptr<TreeNode> dummyNode;
        MAKE_SHARED_RETURN_VALUE(dummyNode, TreeNode, false, dummyEvent);
        dummyNode->records.emplace_back(event);

        if (event->info.start > (*it)->event->info.start && event->info.start <= (*it)->event->info.end) {
            (*it)->children.emplace_back(dummyNode);
            events->Pop();
            matchCnt++;
        } else if (event->info.start > (*it)->event->info.end) {
            // event超过TreeNode的结束时间，需要检查是否在下一个TreeNode的开始之前
            if (std::next(it) != leafNodes.end() && event->info.start < (*std::next(it))->event->info.start) {
                // 挂到rootNode
                missDummyNodes_.emplace_back(dummyNode);
                events->Pop();
                mismatchCnt++;
            }
            it++; // 可能在下一个TreeNode的时间片内，跳过当前TreeNode
        } else {
            // event小于TreeNode的开始时间，记录下来，后面再次校验确认是挂到root还是model
            missDummyNodes_.emplace_back(dummyNode);
            events->Pop();
            mismatchCnt++;
        }
    }
    INFO("Add TaskTrackEvents done, threadId = %, matchCnt = %, mismatchCnt = % ",
         threadId_, matchCnt, mismatchCnt);
    // events没有匹配完
    if (!events->Empty()) {
        INFO("After matching TaskTrackEvents is not empty, size = %, threadId = %", events->GetSize(), threadId_);
        AddRemainTaskTrackEvents(events);
    }
    return true;
}

bool TreeBuilder::AddMissedDummyEvents(std::shared_ptr<TreeNode> &rootNode,
                                       std::vector<std::shared_ptr<TreeNode>> &modelNodes,
                                       std::vector<std::shared_ptr<TreeNode>> &missDummyNodes) const
{
    if (modelNodes.empty() || missDummyNodes.empty()) {
        INFO("No dummy node need to Calibrate, threadId = %", threadId_);
        return false;
    }
    auto mnIt = modelNodes.begin();
    auto mdIt = missDummyNodes.begin();
    uint32_t matchCnt = 0;
    uint32_t mismatchCnt = 0;
    while (mnIt != modelNodes.end() && mdIt != missDummyNodes.end()) {
        if ((*mdIt)->event->info.start > (*mnIt)->event->info.start &&
            (*mdIt)->event->info.start <= (*mnIt)->event->info.end) {
            // 挂到model
            (*mnIt)->children.emplace_back((*mdIt));
            mdIt++;
            matchCnt++;
        } else if ((*mdIt)->event->info.start > (*mnIt)->event->info.end) {
            // event超过TreeNode的结束时间，需要检查是否在下一个TreeNode的开始之前
            if (std::next(mnIt) != modelNodes.end() &&
                (*mdIt)->event->info.start < (*std::next(mnIt))->event->info.start) {
                // 挂到rootNode
                rootNode->children.emplace_back((*mdIt));
                mismatchCnt++;
                mdIt++;
            }
            mnIt++; // 可能在下一个TreeNode的时间片内，跳过当前TreeNode
        } else {
            // event小于TreeNode的开始时间，挂到root
            rootNode->children.emplace_back((*mdIt));
            mdIt++;
            mismatchCnt++;
        }
    }
    while (mdIt != missDummyNodes.end()) {
        rootNode->children.emplace_back((*mdIt));
        mdIt++;
        mismatchCnt++;
    }
    INFO("AddMissedDummyEvents done, threadId = %, matchCnt = %, mismatchCnt = % ",
         threadId_, matchCnt, mismatchCnt);
    return true;
}

void TreeBuilder::AddRemainTaskTrackEvents(std::shared_ptr<EventQueue> &events)
{
    while (!events->Empty()) {
        auto event = events->Top();
        // 建立dummy节点
        EventInfo dummyInfo{EventType::EVENT_TYPE_DUMMY, MSPROF_REPORT_RUNTIME_LEVEL,
                            event->info.start, event->info.end};
        std::shared_ptr<MsprofApi> api;
        MAKE_SHARED0_RETURN_VOID(api, MsprofApi);
        std::shared_ptr<Event> dummyEvent;
        MAKE_SHARED_RETURN_VOID(dummyEvent, Event, api, dummyInfo);
        std::shared_ptr<TreeNode> dummyNode;
        MAKE_SHARED_RETURN_VOID(dummyNode, TreeNode, dummyEvent);
        dummyNode->records.emplace_back(event);

        // 存入miss
        missDummyNodes_.emplace_back(dummyNode);
        events->Pop();
    }
}

void TreeBuilder::RecordTreeNode(const std::shared_ptr<TreeNode> &treeNode,
                                 const uint16_t &eventLevel)
{
    if (eventLevel == MSPROF_REPORT_MODEL_LEVEL) {
        modelLevelNodes_.emplace_back(treeNode);
    } else if (eventLevel == MSPROF_REPORT_NODE_LEVEL) {
        nodeLevelNodes_.emplace_back(treeNode);
    } else if (eventLevel == MSPROF_REPORT_HCCL_NODE_LEVEL) {
        hcclLevelNodes_.emplace_back(treeNode);
    } else if (eventLevel == MSPROF_REPORT_RUNTIME_LEVEL) {
        runtimeLevelNodes_.emplace_back(treeNode);
    }
}

std::shared_ptr<TreeNode> TreeBuilder::BuildTree(std::shared_ptr<TreeNode> parent, int depth)
{
    if (depth >= MAX_DEPTH) {
        ERROR("The maximum recursion depth is exceeded, thread id: %", threadId_);
        return nullptr;
    }
    if (!parent) {
        ERROR("Parent pointer is nullptr, thread id: %", threadId_);
        return nullptr;
    }

    while (true) {
        auto checkEvent = kernelEvents_->Top();
        if (!checkEvent || checkEvent->info.end > parent->event->info.end) {
            if (parent->children.empty()) {
                leafNodes_.emplace_back(parent);
            }
            break;
        }
        auto event = kernelEvents_->Pop();
        std::shared_ptr<TreeNode> childNode;
        MAKE_SHARED_RETURN_VALUE(childNode, TreeNode, nullptr, event);
        // 记录除了Api和TaskTrack类型的TreeNode，按照level记录
        RecordTreeNode(childNode, event->info.level);

        auto childTree = BuildTree(childNode, depth + 1);
        if (childTree != nullptr) {
            childTree->parent = parent;
            parent->children.emplace_back(childTree);
        }
    }
    return parent;
}

} // namespace Cann
} // namespace Association
} // namespace Analysis