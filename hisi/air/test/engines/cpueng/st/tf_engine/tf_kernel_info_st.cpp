#include <gtest/gtest.h>

#define protected public
#define private public
#include "tf_engine/engine/tf_engine.h"
#include "tf_kernel_info/tf_kernel_info.h"
#undef private
#undef protected

#include "util/tf_util.h"
#include "config/config_file.h"
#include "stub.h"

#include "util/util_stub.h"
#include "ge/ge_api_types.h"
#include "config/ops_json_file.h"
#include "graph/utils/tensor_utils.h"

using namespace aicpu;
using namespace ge;
using namespace std;

TEST(TfKernelInfo, Initialize_SUCCESS)
{
    map<string, string> options;
    options[SOC_VERSION] = "Ascend910";
    ASSERT_EQ(Initialize(options), SUCCESS);
    string kernelConfig;
    ASSERT_EQ(ConfigFile::GetInstance().GetValue("DNN_VM_AICPUOpsKernel", kernelConfig), true);
    ASSERT_EQ(kernelConfig, "TFKernel");
    string optimizerConfig;
    ASSERT_EQ(ConfigFile::GetInstance().GetValue("DNN_VM_AICPUGraphOptimizer", optimizerConfig), true);
    ASSERT_EQ(optimizerConfig, "TFOptimizer");
    map<string, OpsKernelInfoStorePtr> opsKernelInfoStores;
    GetOpsKernelInfoStores(opsKernelInfoStores);
    ASSERT_NE(opsKernelInfoStores["aicpu_tf_kernel"], nullptr);
    ASSERT_EQ(opsKernelInfoStores["aicpu_tf_kernel"]->Initialize(options), SUCCESS);
}

TEST(TfKernelInfo, GetValue_FAIL)
{
    map<string, string> options;
    options[SOC_VERSION] = "Ascend910";
    ASSERT_EQ(Initialize(options), SUCCESS);
    string kernelConfig;
    ASSERT_EQ(ConfigFile::GetInstance().GetValue("DNN_VM_AICPUOps", kernelConfig), false);
}

TEST(TfKernelInfo, EmptyOption_FAIL)
{
    map<string, string> options;
    ASSERT_EQ(Initialize(options), SUCCESS);
    string kernelConfig;
    ASSERT_EQ(ConfigFile::GetInstance().GetValue("DNN_VM_AICPUOpsKernel", kernelConfig), true);
    ASSERT_EQ(kernelConfig, "TFKernel");
    string optimizerConfig;
    ASSERT_EQ(ConfigFile::GetInstance().GetValue("DNN_VM_AICPUGraphOptimizer", optimizerConfig), true);
    ASSERT_EQ(optimizerConfig, "TFOptimizer");
    map<string, OpsKernelInfoStorePtr> opsKernelInfoStores;
    GetOpsKernelInfoStores(opsKernelInfoStores);
    ASSERT_NE(opsKernelInfoStores["aicpu_tf_kernel"], nullptr);
    ASSERT_EQ(opsKernelInfoStores["aicpu_tf_kernel"]->Initialize(options), INPUT_PARAM_VALID);
}

TEST(TfKernelInfo, GetAllOpsKernelInfo_SUCCESS)
{
    map<string, OpsKernelInfoStorePtr> opsKernelInfoStores;
    GetOpsKernelInfoStores(opsKernelInfoStores);
    map<string, OpInfo> infos;
    ASSERT_EQ(infos.size(), 0);
    opsKernelInfoStores["aicpu_tf_kernel"]->GetAllOpsKernelInfo(infos);
    ASSERT_NE(infos.size(), 0);
}

TEST(TfKernelInfo, CheckSupported_SUCCESS)
{
    map<string, OpsKernelInfoStorePtr> opsKernelInfoStores;
    GetOpsKernelInfoStores(opsKernelInfoStores);

    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "Add");
    vector<int64_t> tensorShape = {1,1,3,1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x", tensor1);
    opDescPtr->AddInputDesc("y", tensor1);
    opDescPtr->AddOutputDesc("z", tensor1);
    string unSupportedReason;
    ASSERT_EQ(opsKernelInfoStores["aicpu_tf_kernel"]->CheckSupported(opDescPtr, unSupportedReason), true);
    ASSERT_EQ(unSupportedReason, "");
}

TEST(TfKernelInfo, CheckSupportedEmptyOp_FAIL)
{
    map<string, OpsKernelInfoStorePtr> opsKernelInfoStores;
    GetOpsKernelInfoStores(opsKernelInfoStores);

    OpDescPtr opDescPtr = nullptr;
    string unSupportedReason;
    ASSERT_EQ(opsKernelInfoStores["aicpu_tf_kernel"]->CheckSupported(opDescPtr, unSupportedReason), false);
}

TEST(TfKernelInfo, CheckSupportedEmptyType_FAIL)
{
    map<string, OpsKernelInfoStorePtr> opsKernelInfoStores;
    GetOpsKernelInfoStores(opsKernelInfoStores);
    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "Add");
    opDescPtr->SetType("");
    string unSupportedReason;
    ASSERT_EQ(opsKernelInfoStores["aicpu_tf_kernel"]->CheckSupported(opDescPtr, unSupportedReason), false);
}

TEST(TfKernelInfo, CheckSupported_FAIL)
{
    map<string, OpsKernelInfoStorePtr> opsKernelInfoStores;
    GetOpsKernelInfoStores(opsKernelInfoStores);

    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "Ad");
    vector<int64_t> tensorShape = {1,1,3,1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x", tensor1);
    opDescPtr->AddInputDesc("y", tensor1);
    opDescPtr->AddOutputDesc("z", tensor1);

    string unSupportedReason;
    ASSERT_EQ(opsKernelInfoStores["aicpu_tf_kernel"]->CheckSupported(opDescPtr, unSupportedReason), false);
}

TEST(TfKernelInfo, CheckInputSupport_FAIL)
{
    map<string, OpsKernelInfoStorePtr> opsKernelInfoStores;
    GetOpsKernelInfoStores(opsKernelInfoStores);

    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "Add");
    vector<int64_t> tensorShape1 = {1,1,3,1};
    vector<int64_t> tensorShape2 = {1,2,3};
    vector<int64_t> tensorShape3 = {1};
    GeTensorDesc tensor1(GeShape(tensorShape1), FORMAT_NCHW, DT_UINT64);
    GeTensorDesc tensor2(GeShape(tensorShape2), FORMAT_NCHW, DT_FLOAT);
    GeTensorDesc tensor3(GeShape(tensorShape3), FORMAT_NCHW, DT_FLOAT16);
    opDescPtr->AddInputDesc("x1", tensor1);
    opDescPtr->AddInputDesc("x2", tensor2);
    opDescPtr->AddOutputDesc("y", tensor3);
    string unSupportedReason;
    ASSERT_EQ(opsKernelInfoStores["aicpu_tf_kernel"]->CheckSupported(opDescPtr, unSupportedReason), false);
}

TEST(TfKernelInfo, CheckOutputSupport_FAIL)
{
    map<string, OpsKernelInfoStorePtr> opsKernelInfoStores;
    GetOpsKernelInfoStores(opsKernelInfoStores);

    OpDescPtr opDescPtr = make_shared<OpDesc>("Add", "Add");
    vector<int64_t> tensorShape1 = {1,1,3,1};
    vector<int64_t> tensorShape2 = {1,2,3};
    vector<int64_t> tensorShape3 = {1};
    GeTensorDesc tensor1(GeShape(tensorShape1), FORMAT_NCHW, DT_FLOAT16);
    GeTensorDesc tensor2(GeShape(tensorShape2), FORMAT_NCHW, DT_FLOAT16);
    GeTensorDesc tensor3(GeShape(tensorShape3), FORMAT_NCHW, DT_UINT16);
    opDescPtr->AddInputDesc("x1", tensor1);
    opDescPtr->AddInputDesc("x2", tensor2);
    opDescPtr->AddOutputDesc("y", tensor3);
    string unSupportedReason;
    ASSERT_EQ(opsKernelInfoStores["aicpu_tf_kernel"]->CheckSupported(opDescPtr, unSupportedReason), false);
}

TEST(TfKernelInfo, opsFlagCheck_SUCCESS)
{
    map<string, OpsKernelInfoStorePtr> opsKernelInfoStores;
    GetOpsKernelInfoStores(opsKernelInfoStores);

    OpDescPtr opDescPtr = make_shared<OpDesc>("Add","Add");
    vector<int64_t> tensorShape = {1,1,3,1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x1", tensor1);
    opDescPtr->AddInputDesc("x2", tensor1);
    opDescPtr->AddOutputDesc("y", tensor1);
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    string opsFlag;
    opsKernelInfoStores["aicpu_tf_kernel"]->opsFlagCheck(*(graphPtr->AddNode(opDescPtr)), opsFlag);
    ASSERT_EQ(opsFlag, "0");
}

TEST(TfKernelInfo, opsFlagCheck_FAIL)
{
    map<string, OpsKernelInfoStorePtr> opsKernelInfoStores;
    GetOpsKernelInfoStores(opsKernelInfoStores);

    OpDescPtr opDescPtr = make_shared<OpDesc>("Add","Add");
    vector<int64_t> tensorShape = {1,1,3,1};
    GeTensorDesc tensor1(GeShape(tensorShape), FORMAT_NCHW, DT_INT32);
    opDescPtr->AddInputDesc("x", tensor1);
    opDescPtr->AddInputDesc("y", tensor1);
    opDescPtr->AddOutputDesc("z", tensor1);
    opDescPtr->SetType("");
    shared_ptr<ComputeGraph> graphPtr = make_shared<ComputeGraph>("test_graph");
    string opsFlag;
    opsKernelInfoStores["aicpu_tf_kernel"]->opsFlagCheck(*(graphPtr->AddNode(opDescPtr)), opsFlag);
    ASSERT_EQ(opsFlag, "");
}
