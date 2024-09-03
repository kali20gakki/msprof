/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : parser_manager.h
 * Description        : Manager Parser.
 * Author             : msprof team
 * Creation Date      : 2024/08/14
 * *****************************************************************************
*/
#ifndef MSPTI_PARSER_PARSER_MANAGER_H
#define MSPTI_PARSER_PARSER_MANAGER_H

#include <map>
#include <tuple>
#include <mutex>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "common/inject/profapi_inject.h"
#include "activity/ascend/channel/channel_data.h"
#include "external/mspti_activity.h"

namespace Mspti {
namespace Parser {

// <deviceId, streamId, taskId>
using DstType = std::tuple<int16_t, int16_t, int16_t>;
// <deviceId, streamId, flipId>
using DsfType = std::tuple<int16_t, int16_t, int16_t>;

class ParserManager final {
public:
    static ParserManager* GetInstance();

    msptiResult ReportRtTaskTrack(const MsprofRuntimeTrack& track);
    msptiResult ReportStarsSocLog(uint32_t deviceId, const StarsSocLog* socLog);
    void ReportStepTrace(uint32_t deviceId, const StepTrace* stepTrace);
    void ReportFlipInfo(uint32_t deviceId, const TaskFlipInfo* flipInfo);
    msptiResult ReportApi(const MsprofApi* const data);
    uint64_t GenHashId(const std::string &hashInfo);
    std::string& GetHashInfo(uint64_t hashId);
    void RegReportTypeInfo(uint16_t level, uint32_t typeId, const std::string& typeName);
    std::string& GetTypeName(uint16_t level, uint32_t typeId);

private:
    ParserManager() = default;
    explicit ParserManager(const ParserManager &obj) = delete;
    ParserManager& operator=(const ParserManager &obj) = delete;
    explicit ParserManager(ParserManager &&obj) = delete;
    ParserManager& operator=(ParserManager &&obj) = delete;

private:
    // map<<deviceId, streamId, taskId>, msptiActivityKernel*>
    std::map<DstType, std::unique_ptr<msptiActivityKernel>> kernel_map_;
    std::mutex kernel_mtx_;
    
    // map<<deviceId, streamId, flipId>, vector<dstKey>>
    std::map<DsfType, std::vector<DstType>> flip_dst_map_;
    std::mutex flip_map_mtx_;

    // <hashID, hashInfo>
    static std::unordered_map<uint64_t, std::string> hashInfo_map_;
    static std::mutex hashMutex_;

    // <level, <typeid, typename>>
    static std::unordered_map<uint16_t, std::unordered_map<uint32_t, std::string>> typeInfo_map_;
    static std::mutex typeInfoMutex_;
};
}  // Parser
}  // Mspti

#endif
