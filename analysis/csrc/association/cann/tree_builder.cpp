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

#include "tree_builder.h"
#include "event.h"
#include "log.h"
#include "utils.h"

namespace Analysis {
namespace Association {
namespace Cann {

namespace {
const int MAX_DEPTH = 20;
}
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
    if (!cannWarehouse_->kernelEvents) {
        WARN("CANNWarehouse kernelEvents pointer is nullptr, threadId = %", threadId_);
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

    // rootNode的时间片应该覆盖整个EventQueue
    EventInfo rootInfo{EventType::EVENT_TYPE_API, 0, 0, kernelEvents_->GetBound() + 1};
    auto rootEvent = Utils::MakeShared<Event>(std::shared_ptr<MsprofApi>{},
                                              "RootEvent", rootInfo);
    auto rootNode = Utils::MakeShared<TreeNode>(rootEvent);
    // 1. 用Api和TaskTrack Event建立核心树
    auto tree = BuildTree(rootNode, 0);

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
        AddLevelEvents(graphIdMapEvents, modelLevelNodes_);
        AddLevelEvents(fusionOpInfoEvents, modelLevelNodes_);
    });

    // Node Level
    pool.AddTask([this, &nodeCtxIdEvents, &tensorInfoEvents, &nodeBasicInfoEvents]() {
        AddLevelEvents(nodeBasicInfoEvents, nodeLevelNodes_);
        AddLevelEvents(tensorInfoEvents, nodeLevelNodes_);
        AddLevelEvents(nodeCtxIdEvents, nodeLevelNodes_);
    });

    // Hccl level
    pool.AddTask([this, &hcclCtxIdEvents, &hcclInfoEvents]() {
        AddLevelEvents(hcclCtxIdEvents, hcclLevelNodes_);
        AddLevelEvents(hcclInfoEvents, hcclLevelNodes_);
    });

    pool.WaitAllTasks();
    pool.Stop();
    // 向叶子节点添加TaskTrack类型的events
    AddTaskTrackEvents(rootNode, taskTrackEvents, leafNodes_);
    INFO("Build Tree End, ThreadId = %", threadId_);
    return tree;
}

// 将ctxIdEvents按Level分为Node和HCCL层
EventQueuePair TreeBuilder::GroupCtxIdEvents(std::shared_ptr<EventQueue> &ctxIdEvents)
{
    uint64_t ctxIdSize = 0;
    if (ctxIdEvents) {
        ctxIdSize = ctxIdEvents->GetSize();
    }
    auto nodeCtxIdEvent = Utils::MakeShared<EventQueue>(threadId_, ctxIdSize);
    auto hcclCtxIdEvent = Utils::MakeShared<EventQueue>(threadId_, ctxIdSize);
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
                                 std::vector<std::shared_ptr<TreeNode>> &levelNodes) const
{
    // 检查输入各Type的Events指针
    if (!events) {
        ERROR("Events pointer is nullptr, threadId = %", threadId_);
        return false;
    }
    // 保证非空
    if (events->Empty() || levelNodes.empty()) {
        ERROR("Events or LevelNodes is empty, threadId = % ", threadId_);
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
                ERROR("Drop event threadId = %, start = %, its start time between "
                      "last TreeNode end and next TreeNode start", threadId_, event->info.start);
            }
            it++;
        } else {
            // event小于TreeNode的开始时间，此event对应的TreeNode丢失
            events->Pop(); // 丢掉此event
            mismatchCnt++;
            ERROR("Drop event threadId = %, start = %, its start time less than TreeNode start time",
                  threadId_, event->info.start);
        }
    }
    INFO("Add LevelEvents done, threadId = %, matchCnt = %, mismatchCnt = % ",
         threadId_, matchCnt, mismatchCnt);
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
                                     std::vector<std::shared_ptr<TreeNode>> &leafNodes) const
{
    // 检查输入Runtime Events指针
    if (!events) {
        ERROR("TaskTrack Events pointer is nullptr, threadId = %", threadId_);
        return false;
    }
    // 保证非空
    if (events->Empty() || leafNodes.empty()) {
        ERROR("TaskTrack Events or LevelNodes is empty, threadId = % ", threadId_);
        return false;
    }
    uint64_t matchCnt = 0; // 在时间片范围内的数量
    uint64_t mismatchCnt = 0; // 在时间片范围外的数量
    auto it = leafNodes.begin();
    while (!events->Empty() && it != leafNodes.end()) {
        auto event = events->Top();
        // 建立虚拟节点
        EventInfo virInfo{EventType::EVENT_TYPE_API, MSPROF_REPORT_RUNTIME_LEVEL,
                          event->info.start, event->info.end};
        auto virEvent = Utils::MakeShared<Event>(std::shared_ptr<MsprofApi>{},
                                                 "VirtualEvent", virInfo);
        auto virNode = Utils::MakeShared<TreeNode>(virEvent);
        virNode->records.emplace_back(event);

        if (event->info.start > (*it)->event->info.start && event->info.start <= (*it)->event->info.end) {
            (*it)->children.emplace_back(virNode);
            events->Pop();
            matchCnt++;
        } else if (event->info.start > (*it)->event->info.end) {
            // event超过TreeNode的结束时间，需要检查是否在下一个TreeNode的开始之前
            if (std::next(it) != leafNodes.end() && event->info.start < (*std::next(it))->event->info.start) {
                // 挂到rootNode
                rootNode->children.emplace_back(virNode);
                events->Pop();
                mismatchCnt++;
            }
            it++; // 可能在下一个TreeNode的时间片内，跳过当前TreeNode
        } else {
            // event小于TreeNode的开始时间，挂到rootNode
            rootNode->children.emplace_back(virNode);
            events->Pop();
            mismatchCnt++;
        }
    }
    INFO("Add TaskTrackEvents done, threadId = %, matchCnt = %, mismatchCnt = % ",
         threadId_, matchCnt, mismatchCnt);
    // events没有匹配完
    if (!events->Empty()) {
        INFO("After matching TaskTrackEvents is not empty, size = %, threadId = %", events->GetSize(), threadId_);
        AddRemainTaskTrackEvents(rootNode, events);
    }
    return true;
}

void TreeBuilder::AddRemainTaskTrackEvents(std::shared_ptr<TreeNode> &rootNode,
                                           std::shared_ptr<EventQueue> &events) const
{
    while (!events->Empty()) {
        auto event = events->Top();
        // 建立虚拟节点
        EventInfo virInfo{EventType::EVENT_TYPE_API, MSPROF_REPORT_RUNTIME_LEVEL,
                          event->info.start, event->info.end};
        auto virEvent = Utils::MakeShared<Event>(std::shared_ptr<MsprofApi>{},
                                                 "VirtualEvent", virInfo);
        auto virNode = Utils::MakeShared<TreeNode>(virEvent);
        virNode->records.emplace_back(event);

        // 直接挂到rootNode
        rootNode->children.emplace_back(virNode);
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
        ERROR("The maximum recursion depth is exceeded");
        return nullptr;
    }
    if (!parent) {
        ERROR("Parent pointer is nullptr");
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
        auto childNode = Utils::MakeShared<TreeNode>(event);
        // 记录除了Api和TaskTrack类型的TreeNode，按照level记录
        RecordTreeNode(childNode, event->info.level);

        auto childTree = BuildTree(childNode, depth + 1);
        childTree->parent = parent;
        parent->children.emplace_back(childTree);
    }
    return parent;
}

} // namespace Cann
} // namespace Association
} // namespace Analysis