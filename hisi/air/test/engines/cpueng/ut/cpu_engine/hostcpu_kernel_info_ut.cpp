#include <gtest/gtest.h>
#include "hostcpu_engine/engine/hostcpu_engine.h"

#include "util/util.h"
#include "config/config_file.h"
#include "stub.h"

#include "ge/ge_api_types.h"

using namespace aicpu;
using namespace ge;
using namespace std;

TEST(HostCpuKernelInfo, Initialize_SUCCESS)
{
    map<string, string> options;
    options[SOC_VERSION] = "Ascend910";
    ASSERT_EQ(Initialize(options), SUCCESS);
    map<string, OpsKernelInfoStorePtr> opsKernelInfoStores;
    GetOpsKernelInfoStores(opsKernelInfoStores);
    ASSERT_NE(opsKernelInfoStores["DNN_VM_HOST_CPU_OP_STORE"], nullptr);
    ASSERT_EQ(opsKernelInfoStores["DNN_VM_HOST_CPU_OP_STORE"]->Initialize(options), SUCCESS);
}
