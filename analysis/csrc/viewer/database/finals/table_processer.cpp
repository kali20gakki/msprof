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

#include "table_processer.h"
#include "thread_pool.h"

namespace Analysis {
namespace Viewer {
namespace Database {

const uint32_t POOLNUM = 2;

bool TableProcesser::Run()
{
    Analysis::Utils::ThreadPool pool(POOLNUM);
    pool.Start();
    for (auto fileDir : profPaths_) {
        pool.AddTask([this, &fileDir]() {
            Process(fileDir);
        });
    }
    pool.WaitAllTasks();
    pool.Stop();
}


} // Database
} // Viewer
} // Analysis