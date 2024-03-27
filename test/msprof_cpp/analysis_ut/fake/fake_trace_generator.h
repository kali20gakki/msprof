/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : fake_trace_generator.h
 * Description        : Trace虚假数据生成
 * Author             : msprof team
 * Creation Date      : 2023/12/1
 * *****************************************************************************
 */


#ifndef TEST_MSPROF_CPP_ANALYSIS_UT_FAKE_FAKE_TRACE_GENERATOR_H
#define TEST_MSPROF_CPP_ANALYSIS_UT_FAKE_FAKE_TRACE_GENERATOR_H

#include <iostream>
#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <fstream>

#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/entities/event.h"
#include "analysis/csrc/entities/event_queue.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/utils/file.h"
#include "analysis/csrc/utils/utils.h"

using EventType = Analysis::Entities::EventType;
using EventInfo = Analysis::Entities::EventInfo;
using Event = Analysis::Entities::Event;
using EventQueue = Analysis::Entities::EventQueue;

// 伪造生成EventQueue相关公用函数
class FakeEventGenerator {
public:
    static void AddApiEvent(std::shared_ptr<EventQueue> &eventQueue, uint16_t level, uint64_t start,
                            uint64_t end, uint32_t reserve = 0, uint64_t item_id = 0)
    {
        EventInfo testInfo{EventType::EVENT_TYPE_API, level, start, end};
        auto api = std::make_shared<MsprofApi>();
        api->magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        api->level = level;
        api->type = static_cast<uint32_t>(EventType::EVENT_TYPE_API);
        api->threadId = 0;
        api->reserve = reserve;
        api->beginTime = static_cast<uint64_t>(start);
        api->endTime = static_cast<uint64_t>(end);
        api->itemId = item_id;
        auto eventPtr = std::make_shared<Event>(api, testInfo);
        eventQueue->Push(eventPtr);
    }

    static void AddTaskTrackEvent(std::shared_ptr<EventQueue> &eventQueue, uint64_t dot, uint64_t taskType)
    {
        const int size = 8;
        EventInfo testInfo{EventType::EVENT_TYPE_TASK_TRACK, MSPROF_REPORT_RUNTIME_LEVEL,
                           dot, dot};
        auto taskTrack = std::make_shared<MsprofCompactInfo>();
        taskTrack->magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        taskTrack->level = MSPROF_REPORT_RUNTIME_LEVEL;
        taskTrack->type = static_cast<uint32_t>(EventType::EVENT_TYPE_TASK_TRACK);
        taskTrack->threadId = 0;
        taskTrack->dataLen = size;
        taskTrack->timeStamp = static_cast<uint64_t>(dot);

        MsprofRuntimeTrack track;
        track.taskType = taskType;
        taskTrack->data.runtimeTrack = track;
        auto eventPtr = std::make_shared<Event>(taskTrack, testInfo);
        eventQueue->Push(eventPtr);
    }

    static void AddNodeBasicEvent(std::shared_ptr<EventQueue> &eventQueue, uint64_t dot,
                                  uint64_t opType = 1, uint32_t task_type = 0)
    {
        EventInfo testInfo{EventType::EVENT_TYPE_NODE_BASIC_INFO, MSPROF_REPORT_NODE_LEVEL, dot, dot};
        auto nodeBasic = std::make_shared<MsprofCompactInfo>();
        nodeBasic->level = MSPROF_REPORT_NODE_LEVEL;
        nodeBasic->timeStamp = dot;

        MsprofNodeBasicInfo node;
        node.opName = dot;
        node.opType = opType;
        node.taskType = task_type;
        nodeBasic->data.nodeBasicInfo = node;
        auto eventPtr = std::make_shared<Event>(nodeBasic, testInfo);
        eventQueue->Push(eventPtr);
    }

    static void AddTensorInfoEvent(std::shared_ptr<EventQueue> &eventQueue, uint64_t dot)
    {
        EventInfo testInfo{EventType::EVENT_TYPE_TENSOR_INFO, MSPROF_REPORT_NODE_LEVEL, dot, dot};
        auto tensor = std::make_shared<ConcatTensorInfo>();
        tensor->level = MSPROF_REPORT_NODE_LEVEL;
        tensor->timeStamp = dot;
        tensor->opName = dot;

        auto eventPtr = std::make_shared<Event>(tensor, testInfo);
        eventQueue->Push(eventPtr);
    }

    // 增加一个node ctx id的Event
    static void AddNodeCtxIdEvent(std::shared_ptr<EventQueue> &eventQueue, uint64_t dot,
                                  std::pair<uint32_t, uint32_t> ctxRange)
    {
        EventInfo testInfo{EventType::EVENT_TYPE_CONTEXT_ID, MSPROF_REPORT_NODE_LEVEL, dot, dot};
        auto addtionInfo = std::make_shared<MsprofAdditionalInfo>();
        uint32_t num = ctxRange.second - ctxRange.first + 1;
        if (num > MSPROF_CTX_ID_MAX_NUM) {
            throw std::runtime_error("ctx id num is illegal");
        }
        addtionInfo->timeStamp = dot;
        addtionInfo->dataLen = num;

        MsprofContextIdInfo ctxId;
        ctxId.opName = dot;
        ctxId.ctxIdNum = num;
        uint32_t ids[MSPROF_CTX_ID_MAX_NUM];
        for (uint32_t i = 0; i < ctxRange.second - ctxRange.first + 1; i++) {
            ids[i] = ctxRange.first + i;
        }
        std::memcpy(ctxId.ctxIds, &ids, sizeof(uint32_t) * num);
        std::memcpy(addtionInfo->data, &ctxId, sizeof(ctxId));

        auto eventPtr = std::make_shared<Event>(addtionInfo, testInfo);
        eventQueue->Push(eventPtr);
    }

    // 增加一个hccl ctx id的Event
    static void AddHcclCtxIdEvent(std::shared_ptr<EventQueue> &eventQueue, uint64_t dot,
                                  uint32_t endCtx, uint32_t startCtx = 0)
    {
        EventInfo testInfo{EventType::EVENT_TYPE_CONTEXT_ID, MSPROF_REPORT_HCCL_NODE_LEVEL, dot, dot};
        auto addtionInfo = std::make_shared<MsprofAdditionalInfo>();
        uint32_t num = 2;

        addtionInfo->timeStamp = dot;
        addtionInfo->dataLen = num;

        MsprofContextIdInfo ctxId;
        ctxId.opName = dot;
        ctxId.ctxIdNum = num;
        uint32_t ids[2];
        ids[0] = startCtx;
        ids[1] = endCtx;
        std::memcpy(ctxId.ctxIds, &ids, sizeof(uint32_t) * num);
        std::memcpy(addtionInfo->data, &ctxId, sizeof(ctxId));

        auto eventPtr = std::make_shared<Event>(addtionInfo, testInfo);
        eventQueue->Push(eventPtr);
    }

    static void AddGraphIdMapEvent(std::shared_ptr<EventQueue> &eventQueue, uint64_t dot)
    {
        EventInfo testInfo{EventType::EVENT_TYPE_GRAPH_ID_MAP, MSPROF_REPORT_MODEL_LEVEL, dot, dot};
        auto eventPtr = std::make_shared<Event>(std::shared_ptr<MsprofAdditionalInfo>{},
                                                testInfo);
        eventQueue->Push(eventPtr);
    }

    static void AddFusionOpInfoEvent(std::shared_ptr<EventQueue> &eventQueue, uint64_t dot)
    {
        EventInfo testInfo{EventType::EVENT_TYPE_FUSION_OP_INFO, MSPROF_REPORT_MODEL_LEVEL, dot, dot};
        auto eventPtr = std::make_shared<Event>(std::shared_ptr<MsprofAdditionalInfo>{},
                                                testInfo);
        eventQueue->Push(eventPtr);
    }

    static void AddFusionOpEvent(std::shared_ptr<EventQueue> &eventQueue, uint64_t dot,
                                 std::shared_ptr<MsprofAdditionalInfo> additionPtr)
    {
        EventInfo testInfo{EventType::EVENT_TYPE_FUSION_OP_INFO, MSPROF_REPORT_MODEL_LEVEL, dot, dot};
        auto eventPtr = std::make_shared<Event>(additionPtr, testInfo);
        eventQueue->Push(eventPtr);
    }

    static void AddHcclInfoEvent(std::shared_ptr<EventQueue> &eventQueue, uint64_t dot, uint32_t ctxId)
    {
        EventInfo testInfo{EventType::EVENT_TYPE_HCCL_INFO, MSPROF_REPORT_HCCL_NODE_LEVEL, dot, dot};
        auto hcclInfo = std::make_shared<MsprofAdditionalInfo>();
        hcclInfo->magicNumber = MSPROF_DATA_HEAD_MAGIC_NUM;
        hcclInfo->level = MSPROF_REPORT_HCCL_NODE_LEVEL;
        hcclInfo->type = static_cast<uint32_t>(EventType::EVENT_TYPE_HCCL_INFO);
        hcclInfo->timeStamp = static_cast<uint64_t>(dot);
        auto hcclTrace = MsprofHcclInfo{};
        hcclTrace.ctxID = ctxId;
        hcclTrace.itemId = dot;
        std::memcpy(hcclInfo->data, &hcclTrace, sizeof(hcclTrace));
        auto eventPtr = std::make_shared<Event>(hcclInfo, testInfo);
        eventQueue->Push(eventPtr);
    }

    static void AddHcclOpEvent(std::shared_ptr<EventQueue> &eventQueue, uint64_t dot,
                               const std::vector<uint16_t> &algList)
    {
        EventInfo testInfo{EventType::EVENT_TYPE_HCCL_OP_INFO, MSPROF_REPORT_NODE_LEVEL, dot, dot};
        auto hcclOp = std::make_shared<MsprofCompactInfo>();
        hcclOp->level = MSPROF_REPORT_NODE_LEVEL;
        hcclOp->timeStamp = dot;

        MsprofHcclOPInfo node;
        node.groupName = dot;
        node.count = dot + dot;
        node.algType = 0;
        const int alg_bit_cnt = 4;
        for (size_t i = 0; i < algList.size(); ++i) {
            node.algType |= (algList[i] << (i * alg_bit_cnt));
        }
        hcclOp->data.hcclopInfo = node;
        auto eventPtr = std::make_shared<Event>(hcclOp, testInfo);
        eventQueue->Push(eventPtr);
    }
};

class FakeTraceGenerator {
public:
    // fakeDataDir为PROF_XXX级目录
    explicit FakeTraceGenerator(std::string fakeDataDir, const uint32_t &sliceSize = 2048 * 1024)
        : fakeDataDir_(std::move(fakeDataDir)), sliceSize_(sliceSize)
    {
        auto err = Analysis::Utils::File::CreateDir(fakeDataDir_);
        if (!err) {
            std::cout << "CreateDir Error" << std::endl;
        }
    }

    template<typename T>
    void WriteBin(std::vector<T> &traces, EventType eventType, bool isAging,
                  const int deviceId = Analysis::Parser::Environment::HOST_ID)
    {
        auto saveDir = CreateFakeDataDir(eventType, deviceId); // 创建PROF_XXX/host or device_<deviceId>/data
        if (saveDir.empty()) {
            return;
        }
        auto baseFileName = GetHostBinName(eventType, isAging);
        uint64_t totalSize = traces.size() * sizeof(T);
        auto it = traces.begin();
        uint32_t fileCnt = 0;
        uint32_t sizeCnt = 0;
        for (uint32_t i = 0; i < totalSize; i += sliceSize_) {
            auto filePath = saveDir + "/" + baseFileName + std::to_string(fileCnt);
            std::ofstream outFile(filePath, std::ios::out | std::ios::binary);
            std::cout << "write bin : " << filePath << std::endl;
            while (it != traces.end()) {
                if (sizeCnt > sliceSize_) {
                    sizeCnt = 0;
                    break;
                }
                outFile.write((Analysis::Utils::CHAR_PTR) &(*it), sizeof(T));
                sizeCnt += sizeof(T);
                it++;
            }
            fileCnt++;
            outFile.close();
        }
    }
private:
    std::string GetHostBinName(const EventType &type, bool isAging)
    {
        if (isAging) {
            return "aging." + hostBinNames_[type];
        }
        return "unaging." + hostBinNames_[type];
    }

    std::string CreateFakeDataDir(const EventType &type, const int deviceId)
    {
        std::string saveDir;
        if (hostBinNames_.find(type) != hostBinNames_.end()) {
            saveDir = Analysis::Utils::File::PathJoin(std::vector<std::string>{
                fakeDataDir_, hostDir_
            });
        } else if (deviceBinNames_.find(type) != deviceBinNames_.end()) {
            saveDir = Analysis::Utils::File::PathJoin(std::vector<std::string>{
                fakeDataDir_, deviceDir_, std::to_string(deviceId)
            });
        } else {
            return "";
        }
        if (!Analysis::Utils::File::CreateDir(saveDir)) {
            std::cout << "CreateDir fail : " << saveDir << std::endl;
            return "";
        }
        if (!Analysis::Utils::File::CreateDir(saveDir + "/data")) {
            std::cout << "CreateDir fail : " << saveDir + "/data" << std::endl;
            return "";
        }
        return saveDir + "/data";
    }
private:
    std::string fakeDataDir_;
    uint32_t sliceSize_;
    std::string hostDir_ = "host";
    std::string deviceDir_ = "device_";

    std::unordered_map<EventType, std::string> deviceBinNames_;
    std::unordered_map<EventType, std::string> hostBinNames_{
        {EventType::EVENT_TYPE_API, "api_event.data.slice_"},
        {EventType::EVENT_TYPE_EVENT, "api_event.data.slice_"},
        {EventType::EVENT_TYPE_FUSION_OP_INFO, "additional.fusion_op_info.slice_"},
        {EventType::EVENT_TYPE_GRAPH_ID_MAP, "additional.graph_id_map.slice_"},
        {EventType::EVENT_TYPE_NODE_BASIC_INFO, "compact.node_basic_info.slice_"},
        {EventType::EVENT_TYPE_CONTEXT_ID, "additional.context_id_info.slice_"},
        {EventType::EVENT_TYPE_HCCL_INFO, "additional.hccl_info.slice_"},
        {EventType::EVENT_TYPE_TASK_TRACK, "compact.task_track.slice_"},
        {EventType::EVENT_TYPE_TENSOR_INFO, "additional.tensor_info.slice_"},
        {EventType::EVENT_TYPE_MEM_CPY, "compact.memcpy_info.slice_"},
        {EventType::EVENT_TYPE_HCCL_OP_INFO, "compact.hccl_op_info.slice_"},
    };
};

#endif // TEST_MSPROF_CPP_ANALYSIS_UT_FAKE_FAKE_TRACE_GENERATOR_H
