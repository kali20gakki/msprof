/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : tree_analyzer.cpp
 * Description        : EventTree分析模块：遍历EventTree生成待落盘的HostTask
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#include "analysis/csrc/association/cann/tree_analyzer.h"

namespace Analysis {
namespace Association {
namespace Cann {

HCCLBigOps &TreeAnalyzer::GetHcclBigOps() {}

HostTasks &TreeAnalyzer::GetTasks() {}

HostTasks &TreeAnalyzer::GetComputeTasks() {}

HostTasks &TreeAnalyzer::GetHCCLTasks() {}

void TreeAnalyzer::Analyze() {}
} // namespace Cann
} // namespace Association
} // namespace Analysis