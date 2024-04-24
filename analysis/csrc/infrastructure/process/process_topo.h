/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : process_topo.h
 * Description        : 流程执行顺序拓扑算法类
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
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