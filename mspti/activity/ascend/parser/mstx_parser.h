/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mstx_parser.h
 * Description        : Manage mark activity.
 * Author             : msprof team
 * Creation Date      : 2025/05/06
 * *****************************************************************************
*/

#ifndef MSPTI_PARSER_MSTX_PARSER_H
#define MSPTI_PARSER_MSTX_PARSER_H

#include <atomic>
#include <unordered_map>
#include <map>
#include <mutex>
#include <memory>

#include "common/inject/runtime_inject.h"
#include "activity/ascend/channel/channel_data.h"

namespace Mspti {
namespace Parser {
class MstxParser {
public:
    static MstxParser* GetInstance();

    msptiResult ReportMark(const char *msg, RtStreamT stream, const char *domain);

    msptiResult ReportRangeStartA(const char *msg, RtStreamT stream, uint64_t &markId, const char *domain);

    msptiResult ReportRangeEnd(uint64_t rangeId);

    void ReportMarkDataToActivity(uint32_t deviceId, const StepTrace* stepTrace);

    bool IsInnerMarker(uint64_t markId);

    msptiResult InnerDeviceStartA(RtStreamT stream, uint64_t& markId);

    msptiResult InnerDeviceEndA(uint64_t rangeId);

private:
    MstxParser() = default;
    explicit MstxParser(const MstxParser &obj) = delete;
    MstxParser& operator=(const MstxParser &obj) = delete;
    explicit MstxParser(MstxParser &&obj) = delete;
    MstxParser& operator=(MstxParser &&obj) = delete;
    const std::string* TryCacheMarkMsg(const char* msg);

private:
    // Marker
    std::atomic<uint64_t> gMarkId_{0};
    static constexpr uint32_t MARK_TAG_ID{11};
    std::mutex rangeInfoMtx_;
    std::unordered_map<uint64_t, RtStreamT> markId2Stream_;
    static std::unordered_map<std::uint64_t, std::string> hashMarkMsg_;
    static std::mutex markMsgMtx_;

    // Inner Marker
    static std::mutex innerMarkerMutex_;
    static std::unordered_map<uint64_t, RtStreamT> innerMarkIds;
};
}
}
#endif // MSPTI_PARSER_MSTX_PARSER_H
