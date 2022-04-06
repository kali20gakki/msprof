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
