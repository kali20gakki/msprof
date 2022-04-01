#include "ops_kernel_info_store_stub.h"
#include <mutex>
#include <unistd.h>

#include "config/ops_json_file.h"

using namespace ge;
using namespace std;

namespace aicpu {
KernelInfoPtr OpskernelInfoStoreStub::instance_ = nullptr;

KernelInfoPtr OpskernelInfoStoreStub::Instance()
{
    static once_flag flag;
    call_once(flag, [&]() {
        instance_.reset(new (nothrow) OpskernelInfoStoreStub);
    });
    return instance_;
}

bool OpskernelInfoStoreStub::ReadOpInfoFromJsonFile()
{
    char *buffer;
    buffer = getcwd(NULL, 0);
    printf("pwd====%s\n", buffer);
    string jsonPath(buffer);
    free(buffer);
    jsonPath += "/air/test/engines/cpueng/stub/config/tf_kernel.json";
    bool ret = OpsJsonFile::Instance().ParseUnderPath(jsonPath, op_info_json_file_);
    return ret;
}

// FACTORY_KERNELINFO_CLASS_KEY(OpskernelInfoStoreStub, "stub")
}
