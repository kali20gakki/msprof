/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : unified_db_manager.cpp
 * Description        : db类型交付件生成
 * Author             : msprof team
 * Creation Date      : 2023/12/07
 * *****************************************************************************
 */

#include "unified_db_manager.h"

#include "log.h"
#include "thread_pool.h"
#include "id_pool.h"
#include "time_utils.h"
#include "table_processer_factory.h"

namespace Analysis {
namespace Viewer {
namespace Database {

std::vector<std::string> dbName = {
    "api_processer"
};

void UnifiedDBManager::Init()
{
    // reportdb name and Path
    reportDBPath_ = output_ + "/report" + "_" + Analysis::Utils::GetFormatLocalTime() + ".db";
    Analysis::Association::Credential::IdPool::GetInstance();
    Analysis::Log::GetInstance().Init(output_);
}


bool UnifiedDBManager::Run()
{
    Init();

    const uint16_t tableProcessors = 5; // 最多有五个线程
    Analysis::Utils::ThreadPool pool(tableProcessors);
    pool.Start();
    for (auto name : dbName) {
        pool.AddTask([this, &name]() {
            std::shared_ptr<TableProcesser> processer =
                TableProcesserFactory::CreateTableProcessor(name, reportDBPath_, profPaths_);
            if (processer == nullptr) {
                ERROR("% is not defined", name);
                return false;
            }
            processer->Run();
        });
    }

    pool.WaitAllTasks();
    pool.Stop();

    return true;
}

}  // Database
}  // Viewer
}  // Analysis

