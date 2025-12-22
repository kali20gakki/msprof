/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : rt_add_info_center.h
 * Description        : runtime 算子补充信息
 * Author             : msprof team
 * Creation Date      : 2025/11/15
 * *****************************************************************************
 */

#ifndef ANALYSIS_PARSER_HOST_CANN_RT_ADD_INFO_CENTER_H
#define ANALYSIS_PARSER_HOST_CANN_RT_ADD_INFO_CENTER_H

#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "analysis/csrc/infrastructure/utils/singleton.h"
#include "analysis/csrc/domain/entities/hal/include/ascend_obj.h"

namespace Analysis {
namespace Domain {
namespace Host {
namespace Cann {
// 该类是runtime 算子补充信息数据单例类，当前仅读取db，不做解析
class RTAddInfoCenter : public Utils::Singleton<RTAddInfoCenter> {
public:
    void Load(const std::string &path);
    RuntimeOpInfo Get(uint16_t deviceId, uint32_t streamId, uint16_t taskId);

private:
    void LoadDB(const std::string &path);
    std::unordered_map<std::string, RuntimeOpInfo> runtimeOpInfoData_;
};  // class RTAddInfoCenter
}  // namespace Cann
}  // namespace Host
}  // namespace Domain
}  // namespace Analysis

#endif // ANALYSIS_PARSER_HOST_CANN_RT_ADD_INFO_CENTER_H
