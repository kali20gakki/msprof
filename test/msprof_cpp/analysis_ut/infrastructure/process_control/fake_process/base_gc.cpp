/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : base_gc.cpp
 * Description        : fake process
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#include "base_gc.h"
#include <mutex>

namespace Analysis {

namespace Ps {

static uint32_t g_count = 0;
static std::mutex g_mutex;
uint32_t GetBaseGcCount()
{
    std::lock_guard<std::mutex> lg(g_mutex);
    return g_count;
}
void ResetBaseGcCount()
{
    std::lock_guard<std::mutex> lg(g_mutex);
    g_count = 0;
}

void BaseGc::CommonFun()
{
    std::lock_guard<std::mutex> lg(g_mutex);
    g_count++;
}

}

}