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
#include <string>
#include <fstream>
#include "utils.h"
#include "log.h"
#include "file.h"
#include "event.h"
#include "context.h"

using namespace Analysis::Utils;
using namespace Analysis::Parser::Environment;
using EventType = Analysis::Entities::EventType;

class FakeTraceGenerator {
public:
    // fakeDataDir为PROF_XXX级目录
    explicit FakeTraceGenerator(std::string fakeDataDir, const uint32_t &sliceSize = 2048 * 1024)
        : fakeDataDir_(std::move(fakeDataDir)), sliceSize_(sliceSize)
    {
        auto err = File::CreateDir(fakeDataDir_);
        if (!err) {
            std::cout << "CreateDir Error" << std::endl;
        }
    }

    template<typename T>
    void WriteBin(std::vector<T> &traces, EventType eventType, bool isAging, const int deviceId = HOST_ID)
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
                outFile.write((CHAR_PTR) &(*it), sizeof(T));
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
            saveDir = File::PathJoin(std::vector<std::string> {
                fakeDataDir_, hostDir_
            });
        } else if (deviceBinNames_.find(type) != deviceBinNames_.end()) {
            saveDir = File::PathJoin(std::vector<std::string> {
                fakeDataDir_, deviceDir_, std::to_string(deviceId)
            });
        } else {
            return "";
        }
        if (!File::CreateDir(saveDir)) {
            std::cout << "CreateDir fail : " << saveDir << std::endl;
            return "";
        }
        if (!File::CreateDir(saveDir + "/data")) {
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
    };
};

#endif // TEST_MSPROF_CPP_ANALYSIS_UT_FAKE_FAKE_TRACE_GENERATOR_H
