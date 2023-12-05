/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : tree_builder.h
 * Description        : EventTree建树模块：遍历EventQueue建树
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_ASSOCIATION_CANN_TREE_BUILDER_H
#define ANALYSIS_ASSOCIATION_CANN_TREE_BUILDER_H

#include <memory>
#include <vector>
#include <map>
#include "event.h"
#include "tree.h"
#include "cann_warehouse.h"
#include "event_queue.h"
#include "thread_pool.h"
#include "prof_common.h"

namespace Analysis {
namespace Association {
namespace Cann {

using EventQueue = Analysis::Entities::EventQueue;
using TreeNode = Analysis::Entities::TreeNode;
using Event = Analysis::Entities::Event;
using EventType = Analysis::Entities::EventType;
using EventInfo = Analysis::Entities::EventInfo;
using CANNWarehouse = Analysis::Parser::Host::Cann::CANNWarehouse;
using ThreadPool = Analysis::Utils::ThreadPool;

using EventQueuePair = std::pair<std::shared_ptr<EventQueue>, std::shared_ptr<EventQueue>>;

// 一个TreeBuilder对象负责将一个threadId的Event数据建成一棵树
class TreeBuilder {
public:
    TreeBuilder(std::shared_ptr<CANNWarehouse> &cannWarehouse, const uint32_t threadId)
        : cannWarehouse_(cannWarehouse), threadId_(threadId)
    {}
    // 用api和task_track建核心树
    std::shared_ptr<TreeNode> Build();

    // 向核心树的Model, Node, Hccl 层节点添加附加Event
    bool AddLevelEvents(std::shared_ptr<EventQueue> &events,
                        std::vector<std::shared_ptr<TreeNode>> &levelNodes) const;
    // 向叶子节点添加Runtime层的event
    bool AddTaskTrackEvents(std::shared_ptr<TreeNode> &rootNode,
                            std::shared_ptr<EventQueue> &events,
                            std::vector<std::shared_ptr<TreeNode>> &leafNodes) const;

private:
    // 建树核心逻辑
    std::shared_ptr<TreeNode> BuildTree(std::shared_ptr<TreeNode>, int depth);
    // 将ctxIdEvents按Level分为Node和HCCL层
    EventQueuePair GroupCtxIdEvents(std::shared_ptr<EventQueue> &ctxIdEvents);
    // 建树时记录各个Level的TreeNode
    void RecordTreeNode(const std::shared_ptr<TreeNode> &treeNode, const uint16_t &eventLevel);
    // 处理剩余的TaskTrackEvents，将其挂到rootNode上
    void AddRemainTaskTrackEvents(std::shared_ptr<TreeNode> &rootNode, std::shared_ptr<EventQueue> &events) const;
private:
    // 建树时记录叶子节点，用于在其上添加TaskTrack
    std::vector<std::shared_ptr<TreeNode>> leafNodes_;
    // 用于建树的Model、Node、HCCL Level的Api Type Events
    std::shared_ptr<EventQueue> kernelEvents_;
    // 包含各个类型的events
    std::shared_ptr<CANNWarehouse> cannWarehouse_;
    // 此对象处理的threadId
    uint32_t threadId_ = 0;

    // 建树时Model, Node, Hccl, Runtime 层节点记录
    std::vector<std::shared_ptr<TreeNode>> modelLevelNodes_;
    std::vector<std::shared_ptr<TreeNode>> nodeLevelNodes_;
    std::vector<std::shared_ptr<TreeNode>> hcclLevelNodes_;
    std::vector<std::shared_ptr<TreeNode>> runtimeLevelNodes_;
};
} // namespace Cann
} // namespace Association
} // namespace Analysis
#endif // ANALYSIS_ASSOCIATION_CANN_TREE_BUILDER_H
