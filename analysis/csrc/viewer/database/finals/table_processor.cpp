/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : table_processor.cpp
 * Description        : processor的父类实现类，规定统一流程
 * Author             : msprof team
 * Creation Date      : 2023/12/14
 * *****************************************************************************
 */
#include <utility>
#include <atomic>

#include "analysis/csrc/viewer/database/finals/table_processor.h"

#include "analysis/csrc/utils/thread_pool.h"

namespace Analysis {
namespace Viewer {
namespace Database {

const uint32_t POOLNUM = 2;

TableProcessor::TableProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : reportDBPath_(reportDBPath), profPaths_(profPaths)
{
    MAKE_SHARED0_NO_OPERATION(reportDB_.database, ReportDB);
    MAKE_SHARED_NO_OPERATION(reportDB_.dbRunner, DBRunner, reportDBPath_);
}

TableProcessor::TableProcessor(const std::string &reportDBPath)
    : reportDBPath_(reportDBPath)
{
    MAKE_SHARED0_NO_OPERATION(reportDB_.database, ReportDB);
    MAKE_SHARED_NO_OPERATION(reportDB_.dbRunner, DBRunner, reportDBPath_);
}

bool TableProcessor::Run()
{
    std::atomic<bool> retFlag(true);
    Analysis::Utils::ThreadPool pool(POOLNUM);
    pool.Start();
    for (const auto& fileDir : profPaths_) {
        pool.AddTask([this, fileDir, &retFlag]() {
            retFlag = Process(fileDir) & retFlag;
        });
    }
    pool.WaitAllTasks();
    pool.Stop();
    return retFlag;
}

void TableProcessor::PrintProcessorResult(bool result, const std::string &processorName)
{
    if (result) {
        PRINT_INFO("% run success!", processorName);
    } else {
        PRINT_ERROR("% run failed!", processorName);
    }
}

} // Database
} // Viewer
} // Analysis