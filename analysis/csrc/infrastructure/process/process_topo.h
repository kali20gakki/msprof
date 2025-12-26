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
#ifndef ANALYSIS_INFRASTRUCTURE_PROCESS_PROCESS_TOPO_H
#define ANALYSIS_INFRASTRUCTURE_PROCESS_PROCESS_TOPO_H

#include <vector>
#include <memory>
#include "analysis/csrc/infrastructure/process/include/process_register.h"

namespace Analysis {

namespace Infra {

class ProcessTopo final {
public:
    explicit ProcessTopo(const ProcessCollection& processCollection);
    ~ProcessTopo();

    ProcessCollection BuildProcessControlTopoByChip(uint32_t chipId) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}

}

#endif