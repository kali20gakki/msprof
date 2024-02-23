/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : context.h
 * Description        : 采集的Context信息
 * Author             : msprof team
 * Creation Date      : 2023/11/9
 * *****************************************************************************
 */

#ifndef ANALYSIS_PARSER_ENVIRONMENT_CONTEXT_H
#define ANALYSIS_PARSER_ENVIRONMENT_CONTEXT_H

#include <map>
#include <string>
#include <set>

#include "opensource/json/include/nlohmann/json.hpp"

#include "analysis/csrc/utils/singleton.h"
#include "analysis/csrc/utils/time_utils.h"

namespace Analysis {
namespace Parser {
namespace Environment {
const uint16_t HOST_ID = 64;
const uint16_t DEFAULT_DEVICE_ID = UINT16_MAX;
enum class Chip : uint16_t {
    CHIP_V1_1_0 = 0,
    CHIP_V2_1_0 = 1,
    CHIP_V3_1_0 = 2,
    CHIP_V3_2_0 = 3,
    CHIP_V3_3_0 = 4,
    CHIP_V4_1_0 = 5,
    CHIP_V1_1_1 = 7,
    CHIP_V1_1_2 = 8,
    CHIP_V5_1_0 = 9,
    CHIP_V1_1_3 = 11,
};
// 该类是Context信息单例类，读取device(host)路径下json/log文件及环境信息
// 通过 std::unordered_map<std::string, std::unordered_map<uint16_t, nlohmann::json>> 结构的成员变量context_
// 以prof, deviceId两层进行数据路径分割，将该device目录下的对应json和log进行key值合并，统一整合为一份json对象
// 数据查询以prof（无prof则默认为begin()）和deviceId（必选）进行查找
class Context : public Utils::Singleton<Context> {
public:
    bool Load(const std::set<std::string> &profPaths);
    bool IsAllExport();
    void Clear();
public:
    // 获取start_info end_info中的时间
    bool GetProfTimeRecordInfo(Utils::ProfTimeRecord &record, const std::string &profPath = "");
    // 返回info.json 中的pid
    uint32_t GetPidFromInfoJson(uint16_t deviceId = DEFAULT_DEVICE_ID, const std::string &profPath = "");
    // 返回samplejson.json 中的msprofBinPid
    int64_t GetMsBinPid(const std::string &profPath);
    // 获取start_log中的相关时间
    bool GetSyscntConversionParams(Utils::SyscntConversionParams &params, uint16_t deviceId = DEFAULT_DEVICE_ID,
                                   const std::string &profPath = "");
public:
    // 获取对应device的芯片型号
    uint16_t GetPlatformVersion(uint16_t deviceId = DEFAULT_DEVICE_ID, const std::string &profPath = "");
    // 判断芯片类型
    static bool IsStarsChip(uint16_t platformVersion);
    // 校验是否为CHIP_V1_1_x系列(不包含CHIP_V1_1_0)
    static bool IsChipV1(uint16_t platformVersion);
    static bool IsChipV4(uint16_t platformVersion);

private:
    nlohmann::json GetInfoByDeviceId(uint16_t deviceId = DEFAULT_DEVICE_ID, const std::string &profPath = "");
    bool LoadJsonData(const std::string &profPath, const std::string &deviceDir, uint16_t deviceId);
    bool LoadLogData(const std::string &profPath, const std::string &deviceDir, uint16_t deviceId);
    bool CheckInfoValueIsValid(const std::string &profPath, uint16_t deviceId);
    std::unordered_map<std::string, std::map<uint16_t, nlohmann::json>> context_;
};  // class Context
}  // namespace Environment
}  // namespace Parser
}  // namespace Analysis
#endif // ANALYSIS_PARSER_ENVIRONMENT_CONTEXT_H