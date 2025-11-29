/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
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
