/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : table_processer.cpp
 * Description        : processer的父类实现类，规定统一流程
 * Author             : msprof team
 * Creation Date      : 2023/12/14
 * *****************************************************************************
 */
#include <utility>
#include <atomic>

#include "analysis/csrc/viewer/database/finals/table_processer.h"

#include "analysis/csrc/utils/thread_pool.h"

namespace Analysis {
namespace Viewer {
namespace Database {

const uint32_t POOLNUM = 2;

TableProcesser::TableProcesser(std::string reportDBPath, const std::set<std::string> &profPaths)
    : reportDBPath_(std::move(reportDBPath)), profPaths_(profPaths)
{
    MAKE_SHARED0_NO_OPERATION(reportDB_.database, ReportDB);
    MAKE_SHARED_NO_OPERATION(reportDB_.dbRunner, DBRunner, reportDBPath_);
}

TableProcesser::TableProcesser(std::string reportDBPath)
    : reportDBPath_(std::move(reportDBPath))
{
    MAKE_SHARED0_NO_OPERATION(reportDB_.database, ReportDB);
    MAKE_SHARED_NO_OPERATION(reportDB_.dbRunner, DBRunner, reportDBPath_);
}

bool TableProcesser::Run()
{
    std::atomic<bool> retFlag(true);
    Analysis::Utils::ThreadPool pool(POOLNUM);
    pool.Start();
    for (const auto& fileDir : profPaths_) {
        pool.AddTask([this, fileDir, &retFlag]() {
            retFlag = retFlag && Process(fileDir);
        });
    }
    pool.WaitAllTasks();
    pool.Stop();
    return retFlag;
}


} // Database
} // Viewer
} // Analysis