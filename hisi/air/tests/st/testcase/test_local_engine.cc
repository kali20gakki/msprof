/**
 * Copyright 2021 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include "local_engine/engine/host_cpu_engine.h"
#include "external/ge/ge_api.h"
#include "framework/common/types.h"

#include "framework/ge_running_env/src/env/ge_default_running_env.h"
#include "framework/ge_running_env/include/ge_running_env/fake_op.h"
#include "framework/ge_running_env/include/ge_running_env/ge_running_env_faker.h"

#include "ge_graph_dsl/graph_dsl.h"
#include "ge_graph_dsl/assert/graph_assert.h"

namespace ge {
namespace {
class RealDivHostCpuOp : public HostCpuOp{
public:
    RealDivHostCpuOp() {};
    virtual graphStatus Compute(Operator &op,
                                const std::map<std::string, const Tensor> &inputs,
                                std::map<std::string, Tensor> &outputs) {
      return GRAPH_SUCCESS;
    }
};
}

class HostCpuEngineTest : public testing::Test {
 protected:
  void SetUp() {
    REGISTER_HOST_CPU_OP_BUILDER(REALDIV, RealDivHostCpuOp);

    ge_env.InstallDefault()
          .Install(FakeEngine("DNN_VM_AICPU_ASCEND").KernelInfoStore("aicpu_ascend_kernel"))
          .Install(FakeOp(REALDIV).InfoStoreAndBuilder("aicpu_ascend_kernel"));
  }
  void TearDown() {}
  GeRunningEnvFaker ge_env;
};

TEST_F(HostCpuEngineTest, host_cpu_engine_run) {
  vector<int64_t> perm1{1};
  GeTensorDesc tensor_desc1(GeShape(vector<int64_t>{1}), FORMAT_ND, DT_INT64);
  GeTensorPtr const_tensor1 = 
    std::make_shared<GeTensor>(tensor_desc1, reinterpret_cast<uint8_t *>(perm1.data()) , sizeof(int64_t)*perm1.size());
  auto const1 = OP_CFG(CONSTANT).Weight(const_tensor1);

  vector<int32_t> perm2{1};
  GeTensorDesc tensor_desc2(GeShape(vector<int64_t>{1}), FORMAT_ND, DT_INT32);
  GeTensorPtr const_tensor2 = 
    std::make_shared<GeTensor>(tensor_desc2, reinterpret_cast<uint8_t *>(perm2.data()), sizeof(int32_t)*perm2.size());
  auto const2 = OP_CFG(CONSTANT).Weight(const_tensor2);

  DEF_GRAPH(g1) {
    CHAIN(NODE("const1", const1)->EDGE(0, 0)->NODE("realdiv1", REALDIV));
    CHAIN(NODE("const2", const2)->EDGE(0, 1)->NODE("realdiv1", REALDIV));
    CHAIN(NODE("data", DATA)->NODE("realdiv2", REALDIV));
    CHAIN(NODE("realdiv1", REALDIV)->NODE("realdiv2", REALDIV));
    CHAIN(NODE("realdiv2", REALDIV)->NODE("netoutput", NETOUTPUT));
  };

  map<AscendString, AscendString> options;
  EXPECT_EQ(GEInitialize(options), SUCCESS);
  Session session(options);
  session.AddGraph(1, ToGeGraph(g1), options);
  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(GEFinalize(), SUCCESS);
}
}  // namespace ge