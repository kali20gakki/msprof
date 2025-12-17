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
#include "errno/error_code.h"
#include "collect_io_server_stub.h"

using namespace analysis::dvvp::common::error;

int ProfilerServerInit(const std::string &local_sock_name) {
    return PROFILING_SUCCESS;
}

int RegisterSendData(const std::string &name, int (*func)(CONST_VOID_PTR, uint32_t)) {
    return PROFILING_SUCCESS;
}

int control_profiling(const char* uuid, const char* config, uint32_t config_len) {
    return PROFILING_SUCCESS;
}

int profiler_server_send_data(const void* ctx, const void* pkt, uint32_t size)
{
    return PROFILING_SUCCESS;
}

int ProfilerServerDestroy() {
    return PROFILING_SUCCESS;
}
