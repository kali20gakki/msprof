/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : context.cpp
 * Description        : 采集的Context信息
 * Author             : msprof team
 * Creation Date      : 2023/11/20
 * *****************************************************************************
 */

#include "context.h"

#include "file.h"
#include "error_code.h"
#include "log.h"
#include "utils.h"

namespace Analysis {
namespace Parser {
namespace Environment {
using namespace Analysis;
using namespace Analysis::Utils;

namespace {
const uint32_t ALL_EXPORT_VERSION = 0x072211;  // 2023年10月30号之后的驱动版本号
}

bool Context::Load(const std::string &path, uint16_t deviceId)
{
    std::vector<std::string> files = File::GetOriginData(path, filePrefix_, {"done"});
    for (const auto &file : files) {
        FileReader fd(file);
        nlohmann::json content;
        if (fd.ReadJson(content) != ANALYSIS_OK) {
            ERROR("Load context failed: '%'.", file);
            return false;
        }
        context_[deviceId].merge_patch(content);
    }
    return true;
}

bool Context::IsAllExport()
{
    if (context_.empty()) {
        return false;
    }
    const auto &info = context_.begin()->second;
    auto drvVersion = info.value("drvVersion", 0u);
    if (drvVersion < ALL_EXPORT_VERSION) {
        return false;
    }
    uint16_t chip;
    if (StrToU16(chip, info.value("platform_version", "0")) != ANALYSIS_OK) {
        ERROR("str to uint16_t failed");
        return false;
    }
    if (chip == static_cast<uint16_t>(Chip::CHIP_V1_1_0) ||
            chip == static_cast<uint16_t>(Chip::CHIP_V3_1_0) ||
            chip == static_cast<uint16_t>(Chip::CHIP_V1_1_3)) {
        return false;
    }
    return true;
}

void Context::Clear()
{
    context_.clear();
}
}  // namespace Environment
}  // namespace Parser
}  // namespace Analysis