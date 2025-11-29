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
 * -------------------------------------------------------------------------*/
#ifndef ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_PROCESS_STRUCT_H
#define ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_PROCESS_STRUCT_H

#include "analysis/csrc/infrastructure/process/include/process.h"

namespace Analysis {

namespace Infra {

using ProcessCreator = std::function<std::unique_ptr<Process>()>;

struct RegProcessInfo {
    ProcessCreator creator;  // 本Process的创建函数
    std::vector<std::type_index> paramTypes;  // 本Process需要的数据类型
    std::vector<std::type_index> processDependence;  // 本Process依赖的前向Process
    std::vector<uint32_t> chipIds;  // 本Process支持的ChipId
    std::string processName;  // 本Process的类名for Dfx
    bool mandatory;  // 是否为关键流程，关键流程失败后，会停止后续流程导致整体失败 true表示关键流程
};

using ProcessCollection = std::unordered_map<std::type_index, RegProcessInfo>;
}
}


#endif