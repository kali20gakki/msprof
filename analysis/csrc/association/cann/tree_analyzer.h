/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : tree_analyzer.h
 * Description        : Tree分析模块：遍历Tree生成待落盘的HostTask
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_ASSOCIATION_CANN_TREE_ANALYZER_H
#define ANALYSIS_ASSOCIATION_CANN_TREE_ANALYZER_H

#include <memory>
#include <queue>
#include <map>
#include <unordered_map>

#include "analysis/csrc/entities/ascend_obj.h"
#include "analysis/csrc/entities/event.h"
#include "analysis/csrc/entities/tree.h"
#include "analysis/csrc/parser/host/cann/type_data.h"
#include "analysis/csrc/utils/utils.h"

namespace Analysis {
namespace Association {
namespace Cann {

using HostTask = Analysis::Entities::HostTask;
using HostTasks = std::vector<std::shared_ptr<HostTask>>;
using Operator = Analysis::Entities::Operator;
// HCCL大算子描述信息
using HCCLBigOpDescs = std::vector<std::shared_ptr<Operator>>;
// 计算类算子描述信息，Key为opName和timestamp拼接成的字符串
using ComputeOpDescs = std::unordered_map<std::string, std::shared_ptr<Operator>>;
// HCCL小算子描述信息，Key为ctxId
using HCCLSmallOpDescs = std::unordered_map<uint32_t, std::shared_ptr<Operator>>;

class TreeAnalyzer {
public:
    using TypeData = Analysis::Parser::Host::Cann::TypeData;
    using TreeNode = Analysis::Entities::TreeNode;
    using Event = Analysis::Entities::Event;
    using EventType = Analysis::Entities::EventType;

    TreeAnalyzer(const std::shared_ptr<TreeNode> &node, uint32_t threadId)
        : root_(node), threadId_(threadId)
    {}
    // 入口函数
    void Analyze();
    // 获取HCCL相关Tasks数据
    HostTasks &GetHCCLTasks();
    // 获取计算类Tasks数据
    HostTasks &GetComputeTasks();
    // 获取所有Tasks数据
    HostTasks &GetTasks();
    // 获取HCCL大算子数据
    HCCLBigOpDescs &GetHcclBigOps();

private:
    // 更新计算类算子模板函数
    template<typename T, std::shared_ptr<T> Entities::OpDesc::*element>
    void UpdateComputeOpDescs(ComputeOpDescs &opDescs, const std::shared_ptr<T> &trace, uint64_t opName)
    {
        std::string key = Utils::Join("_", opName, trace->timeStamp);
        if (opDescs.find(key) == opDescs.end()) {
            std::shared_ptr<Entities::OpDesc> desc;
            MAKE_SHARED0_RETURN_VOID(desc, Entities::OpDesc);
            (*desc).*element = trace;
            std::shared_ptr<Operator> op;
            MAKE_SHARED_RETURN_VOID(op, Operator, desc, opName,
                                    Entities::OpType::OPTYPE_COMPUTE);
            opDescs.insert({key, op});
        } else {
            auto desc = opDescs[key]->opDesc;
            (*desc).*element = trace;
        }
    }

private:
    // DFS遍历整棵树，记录分析树节点信息保存为落盘数据
    void DeepFirstSearch(const std::shared_ptr<TreeNode> &node);
    // 分析TreeNode入口，此函数内根据其所处的level走不同的逻辑
    void AnalyzeNode(const std::shared_ptr<TreeNode> &node);
    // 分析Runtime层的TreeNode，当DFS到Runtime层时进入此函数
    void AnalyzeRuntimeNode(const std::shared_ptr<TreeNode> &node);

    // 判断当前task是否属于通讯类
    bool IsHcclTask();
    // 判断当前task是否属于计算类
    bool IsComputeTask(const std::shared_ptr<TreeNode> &node);

    // 获取当前计算类节点的信息
    HostTasks GetComputeTaskDescs(const std::shared_ptr<TreeNode> &node);
    // 获取HCCL类节点的信息
    HostTasks GetHcclTaskDescs(const std::shared_ptr<TreeNode> &node);
    // 获取其他节点的信息
    std::shared_ptr<HostTask> GetOtherTaskDesc(const std::shared_ptr<TreeNode> &node);
    // 获取算子信息，key为OpName + timeStamp
    ComputeOpDescs GetComputeOpDescs(const std::shared_ptr<TreeNode> &nodeNode);
    // 获取TreeNode中指定EventType的Records
    std::vector<std::shared_ptr<Event>> GetNodeRecordsByType(const std::shared_ptr<TreeNode> &node,
                                                             const EventType &type);
    // 获取HcclNode信息
    HCCLSmallOpDescs GetHcclSmallOpDescs(const std::shared_ptr<TreeNode> &hcclNode);
    // 更新Hccl大算子描述信息
    void UpdateHcclBigOpDescs(const std::shared_ptr<TreeNode> &node);
    // 根据ctxId和hcclinfo更新hccl小算子描述信息
    bool UpdateHcclSmallOpDescs(HCCLSmallOpDescs &descs,
                                const std::vector<std::shared_ptr<Event>> &ctxIDRecords,
                                const std::vector<std::shared_ptr<Event>> &hcclInfoRecords);
    // 根据hcclinfo更新hccl小算子描述信息
    bool UpdateHcclSmallOpDescs(HCCLSmallOpDescs &descs,
                                const std::vector<std::shared_ptr<Event>> &hcclInfoRecords);

    // 根据输入参数生成HostTask，该函数纯粹负责HostTask生成
    std::shared_ptr<HostTask> GenHostTask(const std::shared_ptr<MsprofCompactInfo> &track,
                                          const std::shared_ptr<MsprofApi> &modelApi,
                                          const std::shared_ptr<Operator> &opPtr,
                                          uint32_t ctxId, uint16_t taskType);
    // 根据输入参数生成HostTask数组，该函数包含了业务场景判断逻辑
    HostTasks GenHostTasks(ComputeOpDescs &ops, const std::shared_ptr<MsprofCompactInfo> &track,
                           const std::shared_ptr<MsprofApi> &modelApi);

private:
    // 树的root节点
    std::shared_ptr<TreeNode> root_;
    // 此对象处理的threadId
    uint32_t threadId_ = 0;
    // 路径记录, key为树的level
    std::map<uint16_t, std::shared_ptr<TreeNode>> path_;
    // 所有Task信息
    HostTasks tasks_;
    // HCCL相关Task信息
    HostTasks hcclTasks_;
    // 计算类Task信息
    HostTasks computeTasks_;
    // HCCL大算子信息
    HCCLBigOpDescs hcclBigOpDescs_;
};

} // namespace Cann
} // namespace Association
} // namespace Analysis
#endif // ANALYSIS_ASSOCIATION_CANN_TREE_ANALYZER_H