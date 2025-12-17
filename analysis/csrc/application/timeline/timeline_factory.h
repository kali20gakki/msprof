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

#ifndef ANALYSIS_APPLICATION_TIMELINE_FACTORY_H
#define ANALYSIS_APPLICATION_TIMELINE_FACTORY_H

#include <functional>
#include "analysis/csrc/application/timeline/json_assembler.h"

namespace Analysis {
namespace Application {
using AssemblerCreator = std::function<void(std::shared_ptr<JsonAssembler> &assembler)>;
class TimelineFactory {
public:
    static std::shared_ptr<JsonAssembler> GetAssemblerByName(const std::string &processName);
private:
    static std::unordered_map<std::string, AssemblerCreator> assemblerTable_;
};
}
}
#endif // ANALYSIS_APPLICATION_TIMELINE_FACTORY_H
