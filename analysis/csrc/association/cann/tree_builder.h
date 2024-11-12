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

#include "analysis/csrc/entities/event.h"
#include "analysis/csrc/entities/event_queue.h"
#include "analysis/csrc/entities/tree.h"
#include "analysis/csrc/parser/host/cann/cann_warehouse.h"
#include "collector/inc/toolchain/prof_common.h"
#include "analysis/csrc/utils/thread_pool.h"

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
    // 多线程添加附加Event
    void MultiThreadAddLevelEvents();
    // 向核心树的Model, Node, Hccl 层节点添加附加Event
    bool AddLevelEvents(std::shared_ptr<EventQueue> &events,
                        std::vector<std::shared_ptr<TreeNode>> &levelNodes,
                        EventType eventType) const;
    // 向叶子节点添加Runtime层的event
    bool AddTaskTrackEvents(std::shared_ptr<TreeNode> &treeNode,
                            std::shared_ptr<EventQueue> &events, uint16_t depth = 1);
    // 返回容纳taskTrackEvent的节点
    std::shared_ptr<TreeNode> MakeDummyNode(const std::shared_ptr<Event>& event);
    // 记录父节点绑定runtime event结果日志
    void LogTreeNode(std::shared_ptr<TreeNode> &treeNode);
private:
    // 建树核心逻辑
    std::shared_ptr<TreeNode> BuildTree(std::shared_ptr<TreeNode>, int depth);
    // 将ctxIdEvents按Level分为Node和HCCL层
    EventQueuePair GroupCtxIdEvents(std::shared_ptr<EventQueue> &ctxIdEvents);
    // 建树时记录各个Level的TreeNode
    void RecordTreeNode(const std::shared_ptr<TreeNode> &treeNode, const uint16_t &eventLevel);
    std::shared_ptr<TreeNode> GenerateRoot();
    static size_t GetEventOffset(size_t idx, std::shared_ptr<Event> &event,
                                 std::vector<std::shared_ptr<TreeNode>> &levelNodes, EventType eventType);
private:
    // 用于建树的Model、Node、HCCL Level的Api Type Events
    std::shared_ptr<EventQueue> kernelEvents_ = nullptr;
    // 包含各个类型的events
    std::shared_ptr<CANNWarehouse> cannWarehouse_ = nullptr;
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
