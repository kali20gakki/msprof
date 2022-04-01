#include <gtest/gtest.h>

#define private public
#define protected public
#include "aicpu_engine/engine/aicpu_engine.h"
#undef protected
#undef private

#include "util/util.h"
#include "config/config_file.h"

#include "ge/ge_api_types.h"

using namespace aicpu;
using namespace ge;

TEST(AicpuEngine, Initialize_SUCCESS)
{
    map<string, string> options;
    options[SOC_VERSION] = "Ascend910";
    ASSERT_EQ(Initialize(options), SUCCESS);
    string kernelConfig;
    ASSERT_EQ(ConfigFile::GetInstance().GetValue("DNN_VM_AICPU_ASCENDOpsKernel", kernelConfig), true);
    ASSERT_EQ(kernelConfig, "CUSTAICPUKernel,AICPUKernel");
    string optimizerConfig;
    ASSERT_EQ(ConfigFile::GetInstance().GetValue("DNN_VM_AICPU_ASCENDGraphOptimizer", optimizerConfig), true);
    ASSERT_EQ(optimizerConfig, "AICPUOptimizer");
    string builderConfig;
    ASSERT_EQ(ConfigFile::GetInstance().GetValue("DNN_VM_AICPU_ASCENDKernelBuilder", builderConfig), true);
    ASSERT_EQ(builderConfig, "AICPUBuilder");
}

TEST(AicpuEngine, GetOpsKernelInfoStores_SUCCESS)
{
    map<string, OpsKernelInfoStorePtr> opsKernelInfoStores;
    GetOpsKernelInfoStores(opsKernelInfoStores);
    ASSERT_NE(opsKernelInfoStores["aicpu_ascend_kernel"], nullptr);
    ASSERT_EQ(((AicpuOpsKernelInfoStore *)(opsKernelInfoStores["aicpu_ascend_kernel"].get()))->engine_name_, "DNN_VM_AICPU_ASCEND");
}

TEST(AicpuEngine, GetGraphOptimizerObjs_SUCCESS)
{
    map<string, GraphOptimizerPtr> graphOptimizers;
    AicpuEngine::Instance()->GetGraphOptimizerObjs(graphOptimizers);
    ASSERT_NE(graphOptimizers["aicpu_ascend_optimizer"], nullptr);
    ASSERT_EQ(((AicpuGraphOptimizer *)(graphOptimizers["aicpu_ascend_optimizer"].get()))->engine_name_, "DNN_VM_AICPU_ASCEND");
}

TEST(AicpuEngine, GetOpsKernelBuilderObjs_SUCCESS)
{
    map<string, OpsKernelBuilderPtr> opsKernelBuilders;
    AicpuEngine::Instance()->GetOpsKernelBuilderObjs(opsKernelBuilders);
    ASSERT_NE(opsKernelBuilders["aicpu_ascend_builder"], nullptr);
    ASSERT_EQ(((AicpuOpsKernelBuilder *)(opsKernelBuilders["aicpu_ascend_builder"].get()))->engine_name_, "DNN_VM_AICPU_ASCEND");
}

TEST(AicpuEngine, Finalize_SUCCESS)
{
    map<string, string> options;
    options[SOC_VERSION] = "Ascend910";
    ASSERT_EQ(Finalize(), SUCCESS);
    ASSERT_EQ(AicpuEngine::Instance()->Initialize(options), SUCCESS);
    ASSERT_EQ(AicpuEngine::Instance()->Finalize(), SUCCESS);
}
