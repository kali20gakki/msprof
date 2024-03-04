/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : tree_analyzer.cpp
 * Description        : EventTree分析模块：遍历EventTree生成待落盘的HostTask
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#include <string>
#include <memory>
#include <set>
#include "analysis/csrc/association/cann/tree_analyzer.h"

namespace Analysis {
namespace Association {
namespace Cann {

using namespace Analysis::Entities;
using namespace Analysis::Utils;

namespace {
const uint32_t VALID_CTXID_NUM = 2;
const int64_t INVALID_VALUE = -1;
const uint16_t TWO_BYTES = 16;
const uint16_t AICORE_TASK_TYPE = 0;
const uint64_t INVALID_MODEL_ID = 4294967295;
const std::string KERNEL_TASK_PREFIX = "KERNEL";
const uint32_t FFTS_PLUS_TASK_TYPE = 6;
const uint32_t HCCL_TASK_TYPE = 9;
const uint32_t HCCL_AI_CPU_TASK_TYPE = 11;
const std::string KERNEL_FFTS_PLUS_TASK_TYPE = "FFTS_PLUS";
const std::string KERNEL_STARS_COMMON_TASK_TYPE = "STARS_COMMON";
const std::string KERNEL_AI_CORE_TASK_TYPE = "KERNEL_AICORE";
const std::string KERNEL_AI_CPU_TASK_TYPE = "KERNEL_AICPU";
const std::set<std::string> CONTEXT_ID_WHITE_LIST = {"KERNEL_AICORE", "KERNEL_AIVEC", "FFTS_PLUS"};
}

void TreeAnalyzer::Analyze()
{
    DeepFirstSearch(root_);
}

HostTasks &TreeAnalyzer::GetHCCLTasks()
{
    return hcclTasks_;
}

HostTasks &TreeAnalyzer::GetComputeTasks()
{
    return computeTasks_;
}

HostTasks &TreeAnalyzer::GetTasks()
{
    return tasks_;
}

HCCLBigOpDescs &TreeAnalyzer::GetHcclBigOps()
{
    return hcclBigOpDescs_;
}

void TreeAnalyzer::DeepFirstSearch(const std::shared_ptr<TreeNode> &node)
{
    if (!node || !node->event) {
        ERROR("Node pointer or event is nullptr, threadId = %", threadId_);
        return;
    }
    AnalyzeNode(node);
    path_[node->event->info.level] = node;
    // DFS
    for (const auto &child: node->children) {
        DeepFirstSearch(child);
    }
    path_.erase(node->event->info.level);
}

void TreeAnalyzer::AnalyzeNode(const std::shared_ptr<TreeNode> &node)
{
    if (node->event->info.level == MSPROF_REPORT_RUNTIME_LEVEL) {
        AnalyzeRuntimeNode(node);
    }
}

void TreeAnalyzer::AnalyzeRuntimeNode(const std::shared_ptr<TreeNode> &node)
{
    auto isHccl = IsHcclTask();
    auto isCompute = IsComputeTask(node);
    if (isCompute) {
        auto computeTasks = GetComputeTaskDescs(node);
        computeTasks_.insert(computeTasks_.end(), computeTasks.begin(), computeTasks.end());
        tasks_.insert(tasks_.end(), computeTasks.begin(), computeTasks.end());
    }

    if (isHccl) {
        auto hcclTasks = GetHcclTaskDescs(node);
        hcclTasks_.insert(hcclTasks_.end(), hcclTasks.begin(), hcclTasks.end());
        for (auto &task : hcclTasks) {
            if (isCompute) {
                // task已插入，无需再次插入
                continue;
            }
            tasks_.emplace_back(task);
        }
        UpdateHcclBigOpDescs(node);
    }

    if (!isCompute && !isHccl) {
        auto otherTask = GetOtherTaskDesc(node);
        if (otherTask) {
            tasks_.emplace_back(otherTask);
        }
    }
}

bool TreeAnalyzer::IsHcclTask()
{
    return path_.find(MSPROF_REPORT_HCCL_NODE_LEVEL) != path_.end();
}

bool TreeAnalyzer::IsComputeTask(const std::shared_ptr<TreeNode> &node)
{
    for (const auto &record: node->records) {
        if (record->info.type != EventType::EVENT_TYPE_TASK_TRACK) {
            continue;
        }
        auto trace = record->compactPtr;
        auto taskType = TypeData::GetInstance().Get(node->event->info.level,
                                                    trace->data.runtimeTrack.taskType);
        if (taskType.substr(0, KERNEL_TASK_PREFIX.size()) == KERNEL_TASK_PREFIX ||
            taskType == KERNEL_STARS_COMMON_TASK_TYPE) {
            // 传统core task和dsa task
            return true;
        } else if (taskType == KERNEL_FFTS_PLUS_TASK_TYPE) {
            // ffts+ task
            return path_.find(MSPROF_REPORT_HCCL_NODE_LEVEL) == path_.end();
        }
        return false;
    }
    return false;
}

void TreeAnalyzer::UpdateHcclBigOpDescs(const std::shared_ptr<TreeNode> &node)
{
    if (path_.find(MSPROF_REPORT_NODE_LEVEL) == path_.end()) {
        return;
    }
    auto nodeNode = path_[MSPROF_REPORT_NODE_LEVEL];
    auto tracks = GetNodeRecordsByType(node, EventType::EVENT_TYPE_TASK_TRACK);
    if (tracks.empty()) {
        ERROR("TreeNode task track records is empty, threadId = %", threadId_);
        return;
    }
    auto track = tracks.back()->compactPtr;
    auto deviceId = track->data.runtimeTrack.deviceId;

    if (!hcclBigOpDescs_.empty() &&
        hcclBigOpDescs_.back()->hcclBigOpDesc->endTime == nodeNode->event->info.end) {
        WARN("Find same end time when update Hccl big op descs, threadId = %", threadId_);
        return;
    }

    auto nodeRecords = GetNodeRecordsByType(nodeNode, EventType::EVENT_TYPE_NODE_BASIC_INFO);
    if (nodeRecords.empty()) {
        auto nodeApi = nodeNode->event->apiPtr;
        std::shared_ptr<HcclBigOpDesc> desc;
        MAKE_SHARED_RETURN_VOID(desc, HcclBigOpDesc, nodeApi->beginTime,
                                nodeApi->endTime, deviceId, nullptr);
        std::shared_ptr<Operator> op;
        MAKE_SHARED_RETURN_VOID(op, Operator, desc, nodeApi->itemId, OpType::OPTYPE_HCCL_BIG);
        hcclBigOpDescs_.emplace_back(op);
        return;
    }

    for (const auto &nodeRecord: nodeRecords) {
        auto nodeApi = nodeNode->event->apiPtr;
        auto nodeDesc = (nodeRecord == nullptr) ? nullptr : nodeRecord->compactPtr;
        std::shared_ptr<HcclBigOpDesc> desc;
        MAKE_SHARED_RETURN_VOID(desc, HcclBigOpDesc, nodeApi->beginTime,
                                nodeApi->endTime, deviceId, nodeDesc);
        std::shared_ptr<Operator> op;
        MAKE_SHARED_RETURN_VOID(op, Operator, desc, nodeApi->itemId, OpType::OPTYPE_HCCL_BIG);
        hcclBigOpDescs_.emplace_back(op);
    }
}

std::vector<std::shared_ptr<Event>> TreeAnalyzer::GetNodeRecordsByType(const std::shared_ptr<TreeNode> &node,
                                                                       const EventType &type)
{
    std::vector<std::shared_ptr<Event>> records;
    for (const auto &record: node->records) {
        if (record->info.type == type) {
            records.emplace_back(record);
        }
    }
    return records;
}

std::shared_ptr<HostTask> TreeAnalyzer::GenHostTask(const std::shared_ptr<MsprofCompactInfo> &track,
                                                    const std::shared_ptr<MsprofApi> &modelApi,
                                                    const std::shared_ptr<Operator> &opPtr,
                                                    uint32_t ctxId, uint16_t taskType,
                                                    int64_t connectionId)
{
    std::shared_ptr<HostTask> task;
    MAKE_SHARED0_RETURN_VALUE(task, HostTask, nullptr);

    task->connection_id = connectionId;
    task->streamId = track->data.runtimeTrack.streamId;
    task->taskId = track->data.runtimeTrack.taskId;
    task->contextId = ctxId;
    task->op = opPtr;
    task->modelId = modelApi != nullptr ? modelApi->itemId : INVALID_MODEL_ID;
    task->batchId = static_cast<uint16_t>(track->data.runtimeTrack.taskId >> TWO_BYTES);
    task->requestId = modelApi != nullptr ? modelApi->reserve : INVALID_VALUE;
    task->taskType = taskType;
    task->deviceId = track->data.runtimeTrack.deviceId;
    task->timeStamp = track->timeStamp;
    return task;
}

HostTasks TreeAnalyzer::GenComputeHostTasks(ComputeOpDescs &ops,
                                            const std::shared_ptr<MsprofCompactInfo> &track,
                                            const std::shared_ptr<MsprofApi> &modelApi,
                                            int64_t connection_id)
{
    // L0场景没有上报Node层信息
    if (ops.empty()) {
        auto task = GenHostTask(track, modelApi, nullptr,
                                DEFAULT_CONTEXT_ID, track->data.runtimeTrack.taskType, connection_id);
        return {task};
    }

    HostTasks results;
    for (const auto &pair: ops) {
        auto desc = pair.second->opDesc;
        std::vector<uint32_t> ctxIds;
        // 只有FFTS+类型应该保留CtxId, 过滤Memcpy存在CtxId
        if (desc->ctxId) {
            auto ctxIdInfo = ReinterpretConvert<MsprofContextIdInfo *>(desc->ctxId->data);
            ctxIds.assign(ctxIdInfo->ctxIds, ctxIdInfo->ctxIds + ctxIdInfo->ctxIdNum);
        } else {
            ctxIds = {DEFAULT_CONTEXT_ID};
        }

        // L0 FFTS+场景没有上报NodeBasicInfo, 但是存在ctxIds
        if (desc->nodeDesc == nullptr) {
            for (const auto &ctxId: ctxIds) {
                auto task = GenHostTask(track, modelApi, nullptr, ctxId,
                                        track->data.runtimeTrack.taskType, connection_id);
                results.emplace_back(task);
            }
            continue;
        }

        for (const auto &ctxId: ctxIds) {
            auto task = GenHostTask(track, modelApi, pair.second, ctxId,
                                    track->data.runtimeTrack.taskType, connection_id);
            results.emplace_back(task);
        }
    }
    return results;
}

void TreeAnalyzer::UpdateComputeDescForFftsSituation(ComputeOpDescs &descs,
                                                     const std::shared_ptr<Event> &track)
{
    std::string specialKey;
    // notice： 特殊场景，刷新op描述
    for (auto &pair : descs) {
        if (!pair.second->opDesc) {
            ERROR("Illegal compute op, no op desc found, threadId = %", threadId_);
            continue;
        }
        if (!pair.second->opDesc->nodeDesc) {
            continue;
        }
        // 特殊场景1： ffts+模式会上报一条多余的nodebasic用于标记ffts图，删除
        if (pair.second->opDesc->nodeDesc->data.nodeBasicInfo.taskType == FFTS_PLUS_TASK_TYPE) {
            specialKey = pair.first;
            // 该数据在一个task中只有1条
            break;
        }
    }
    if (!specialKey.empty()) {
        descs.erase(specialKey);
    }
}

void TreeAnalyzer::UpdateComputeDescForHcclSituation(ComputeOpDescs &descs,
                                                     const std::shared_ptr<Event> &track,
                                                     uint32_t item_id)
{
    if (descs.empty()) {
        // 补充一个临时的算子描述，只发生在L0场景
        std::shared_ptr<OpDesc> desc;
        std::shared_ptr<Operator> op;
        MAKE_SHARED0_NO_OPERATION(desc, OpDesc);
        MAKE_SHARED0_NO_OPERATION(op, Operator, desc, UINT64_MAX, OpType::OPTYPE_INVALID);
        descs[std::to_string(UINT64_MAX) + "_" + std::to_string(UINT64_MAX)] = op;
    }
    for (auto &pair : descs) {
        if (!pair.second->opDesc) {
            ERROR("Illegal compute op, no op desc found, threadId = %", threadId_);
            continue;
        }
        if (!pair.second->opDesc->nodeDesc) {
            // 避免内存泄露
            MAKE_SHARED0_NO_OPERATION(pair.second->opDesc->nodeDesc, MsprofCompactInfo);
        }
        // 特殊场景2：hccl类任务将opName刷成item id
        pair.second->opDesc->nodeDesc->data.nodeBasicInfo.opName = item_id;
        auto taskType = track->compactPtr->data.runtimeTrack.taskType;
        auto taskTypeStr = TypeData::GetInstance().Get(MSPROF_REPORT_RUNTIME_LEVEL, taskType);
        if (taskTypeStr == KERNEL_AI_CPU_TASK_TYPE) {
            // 特殊场景3：HCCL AICPU任务，该任务需要将任务类型刷为HCCL_AICPU
            pair.second->opDesc->nodeDesc->data.nodeBasicInfo.taskType = HCCL_AI_CPU_TASK_TYPE;
        } else if (taskTypeStr == KERNEL_AI_CORE_TASK_TYPE) {
            // 特殊场景4：HCCL AICORE(Reduce TBE)任务，该任务需要将任务类型刷为AI_CORE
            pair.second->opDesc->nodeDesc->data.nodeBasicInfo.taskType = HCCL_TASK_TYPE;
        }
    }
}

HostTasks TreeAnalyzer::GetComputeTaskDescs(const std::shared_ptr<TreeNode> &node)
{
    auto nodeNode = path_.find(MSPROF_REPORT_NODE_LEVEL) != path_.end() ?
                    path_[MSPROF_REPORT_NODE_LEVEL] : nullptr;
    auto modelNode = path_.find(MSPROF_REPORT_MODEL_LEVEL) != path_.end() ?
                     path_[MSPROF_REPORT_MODEL_LEVEL] : nullptr;

    auto tracks = GetNodeRecordsByType(node, EventType::EVENT_TYPE_TASK_TRACK);
    if (tracks.empty()) {
        ERROR("TreeNode task track records is empty, threadId = %", threadId_);
        return {};
    }

    std::string type = TypeData::GetInstance().Get(MSPROF_REPORT_RUNTIME_LEVEL,
                                                   tracks.back()->compactPtr->data.runtimeTrack.taskType);
    bool useCtxId = CONTEXT_ID_WHITE_LIST.find(type) != CONTEXT_ID_WHITE_LIST.end();
    ComputeOpDescs ops = GetComputeOpDescs(nodeNode, useCtxId);
    // 特殊场景，刷新算子描述
    UpdateComputeDescForFftsSituation(ops, tracks.back());
    if (IsHcclTask()) {
        auto hcclNode = path_.find(MSPROF_REPORT_HCCL_NODE_LEVEL) != path_.end() ?
                        path_[MSPROF_REPORT_HCCL_NODE_LEVEL] : nullptr;
        auto item_id = hcclNode == nullptr ? 0 : hcclNode->event->apiPtr->itemId;
        UpdateComputeDescForHcclSituation(ops, tracks.back(), item_id);
    }
    auto track = tracks.back()->compactPtr;
    auto modelApi = modelNode != nullptr ?
                    modelNode->event->apiPtr : nullptr;

    auto results = GenComputeHostTasks(ops, track, modelApi, nodeNode->event->id);
    return results;
}

HostTasks TreeAnalyzer::GetHcclTaskDescs(const std::shared_ptr<TreeNode> &node)
{
    if (path_.find(MSPROF_REPORT_HCCL_NODE_LEVEL) == path_.end() ||
        path_.find(MSPROF_REPORT_NODE_LEVEL) == path_.end()) {
        return {};
    }
    auto hcclNode = path_[MSPROF_REPORT_HCCL_NODE_LEVEL];
    auto nodeNode = path_[MSPROF_REPORT_NODE_LEVEL];
    auto modelTrace = path_.find(MSPROF_REPORT_MODEL_LEVEL) != path_.end() ?
                      path_[MSPROF_REPORT_MODEL_LEVEL] : nullptr;
    auto modelApi = modelTrace != nullptr ?
                    modelTrace->event->apiPtr : nullptr;

    auto tracks = GetNodeRecordsByType(node, EventType::EVENT_TYPE_TASK_TRACK);
    if (tracks.empty()) {
        ERROR("TreeNode task track records is empty, threadId = %", threadId_);
        return {};
    }
    auto track = tracks.back()->compactPtr;
    HCCLSmallOpDescs hcclOpDescs = GetHcclSmallOpDescs(hcclNode);

    HostTasks results;
    auto ret = Utils::Reserve(results, hcclOpDescs.size());
    if (!ret) {
        ERROR("Reserve results failed, threadId = %", threadId_);
        return results;
    }

    for (const auto &pair: hcclOpDescs) {
        auto desc = pair.second->hcclSmallOpDesc;
        auto task = GenHostTask(track, modelApi, pair.second, desc->ctxId,
                                track->data.runtimeTrack.taskType, nodeNode->event->id);
        results.emplace_back(task);
    }
    return results;
}

std::shared_ptr<HostTask> TreeAnalyzer::GetOtherTaskDesc(const std::shared_ptr<TreeNode> &node)
{
    auto modelNode = path_.find(MSPROF_REPORT_MODEL_LEVEL) != path_.end() ?
                     path_[MSPROF_REPORT_MODEL_LEVEL] : nullptr;
    auto nodeNode = path_.find(MSPROF_REPORT_NODE_LEVEL) != path_.end() ?
                    path_[MSPROF_REPORT_NODE_LEVEL] : nullptr;
    auto modelApi = modelNode != nullptr ?
                    modelNode->event->apiPtr : nullptr;
    auto tracks = GetNodeRecordsByType(node, EventType::EVENT_TYPE_TASK_TRACK);
    if (tracks.empty()) {
        ERROR("TreeNode task track records is empty, threadId = %", threadId_);
        return nullptr;
    }
    auto track = tracks.back()->compactPtr;

    auto task = GenHostTask(track, modelApi, nullptr, DEFAULT_CONTEXT_ID,
                            track->data.runtimeTrack.taskType, INVALID_VALUE);
    return task;
}

ComputeOpDescs TreeAnalyzer::GetComputeOpDescs(const std::shared_ptr<TreeNode> &nodeNode, bool useCtxId)
{
    ComputeOpDescs opDescs;
    if (!nodeNode) {
        ERROR("nodeNode is nullptr, threadId = %", threadId_);
        return opDescs;
    }
    for (const auto &record: nodeNode->records) {
        if (record->info.type == EventType::EVENT_TYPE_NODE_BASIC_INFO) {
            auto trace = record->compactPtr;
            UpdateComputeOpDescs<MsprofCompactInfo, &OpDesc::nodeDesc>(opDescs, trace,
                                                                       trace->data.nodeBasicInfo.opName);
        } else if (record->info.type == EventType::EVENT_TYPE_TENSOR_INFO) {
            auto trace = record->tensorPtr;
            UpdateComputeOpDescs<ConcatTensorInfo, &OpDesc::tensorDesc>(opDescs, trace,
                                                                        trace->opName);
        } else if (record->info.type == EventType::EVENT_TYPE_CONTEXT_ID) {
            if (!useCtxId) {
                continue;
            }
            auto trace = record->additionPtr;
            auto ctxIdNode = ReinterpretConvert<MsprofContextIdInfo *>(trace->data);
            UpdateComputeOpDescs<MsprofAdditionalInfo, &OpDesc::ctxId>(opDescs, trace,
                                                                       ctxIdNode->opName);
        } else {
            ERROR("Unsupported additional type = %, threadId = %",
                  static_cast<uint16_t>(record->info.type), threadId_);
        }
    }
    return opDescs;
}

HCCLSmallOpDescs TreeAnalyzer::GetHcclSmallOpDescs(const std::shared_ptr<TreeNode> &hcclNode)
{
    HCCLSmallOpDescs opDescs;
    auto ctxIdRecords = GetNodeRecordsByType(hcclNode, EventType::EVENT_TYPE_CONTEXT_ID);
    auto hcclInfoRecords = GetNodeRecordsByType(hcclNode, EventType::EVENT_TYPE_HCCL_INFO);

    if (!ctxIdRecords.empty()) {
        // ctxId 存在，先根据其生成descs, 再补充hccl info
        auto ret = UpdateHcclSmallOpDescs(opDescs, ctxIdRecords, hcclInfoRecords);
        if (!ret) {
            ERROR("Update hccl small op descs failed by ctxId and hcclInfo, threadId = %", threadId_);
        }
    } else if (!hcclInfoRecords.empty()) {
        // 根据hccl info 生成
        auto ret = UpdateHcclSmallOpDescs(opDescs, hcclInfoRecords);
        if (!ret) {
            ERROR("Update hccl small op descs failed by hcclInfo, threadId = %", threadId_);
        }
    } else {
        std::shared_ptr<HcclSmallOpDesc> desc;
        MAKE_SHARED0_RETURN_VALUE(desc, HcclSmallOpDesc, opDescs);
        std::shared_ptr<Operator> op;
        MAKE_SHARED_RETURN_VALUE(op, Operator, opDescs, desc, 0, OpType::OPTYPE_HCCL_SMALL);
        opDescs.insert({DEFAULT_CONTEXT_ID, op});
    }

    return opDescs;
}

bool TreeAnalyzer::UpdateHcclSmallOpDescs(HCCLSmallOpDescs &descs,
                                          const std::vector<std::shared_ptr<Event>> &ctxIdRecords,
                                          const std::vector<std::shared_ptr<Event>> &hcclInfoRecords)
{
    // 根据ctxId生成descs
    for (const auto &record: ctxIdRecords) {
        auto trace = record->additionPtr;
        auto ctxIdTrace = ReinterpretConvert<MsprofContextIdInfo *>(trace->data);
        if (ctxIdTrace->ctxIdNum != VALID_CTXID_NUM) {
            ERROR("Expect ctxIdNum is 2, but get ctxIdNum is %, threadId = %", ctxIdTrace->ctxIdNum, threadId_);
            return false;
        }
        for (uint32_t id = ctxIdTrace->ctxIds[0]; id <= ctxIdTrace->ctxIds[1]; ++id) {
            std::shared_ptr<HcclSmallOpDesc> desc;
            MAKE_SHARED0_RETURN_VALUE(desc, HcclSmallOpDesc, false);
            desc->ctxId = id;
            std::shared_ptr<Operator> op;
            MAKE_SHARED_RETURN_VALUE(op, Operator, false, desc, ctxIdTrace->opName, OpType::OPTYPE_HCCL_SMALL);
            descs.insert({id, op});
        }
    }

    // HcclInfo更新
    for (const auto &record: hcclInfoRecords) {
        auto trace = record->additionPtr;
        auto hcclTrace = ReinterpretConvert<MsprofHcclInfo *>(trace->data);
        auto key = hcclTrace->ctxID;
        if (descs.find(key) != descs.end()) {
            auto hcclPtr = descs[key]->hcclSmallOpDesc;
            hcclPtr->hcclInfo = trace;
        } else {
            ERROR("Can not find ctxId : % in descs, timestamp = %, threadId = %", key, trace->timeStamp, threadId_);
        }
    }
    return true;
}

bool TreeAnalyzer::UpdateHcclSmallOpDescs(HCCLSmallOpDescs &descs,
                                          const std::vector<std::shared_ptr<Event>> &hcclInfoRecords)
{
    for (const auto &record: hcclInfoRecords) {
        auto trace = record->additionPtr;
        auto hcclTrace = ReinterpretConvert<MsprofHcclInfo *>(trace->data);
        auto key = hcclTrace->ctxID;

        std::shared_ptr<HcclSmallOpDesc> desc;
        MAKE_SHARED0_RETURN_VALUE(desc, HcclSmallOpDesc, false);
        desc->hcclInfo = trace;
        std::shared_ptr<Operator> op;
        MAKE_SHARED_RETURN_VALUE(op, Operator, false, desc, hcclTrace->itemId, OpType::OPTYPE_HCCL_SMALL);

        descs.insert({key, op});
        auto hcclPtr = descs[key]->hcclSmallOpDesc;
        hcclPtr->hcclInfo = trace;
    }
    return true;
}

} // namespace Cann
} // namespace Association
} // namespace Analysis