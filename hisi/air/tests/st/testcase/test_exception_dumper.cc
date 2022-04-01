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
#define protected public
#define private public
#include "external/ge/ge_api.h"
#include "graph/compute_graph.h"
#include "framework/common/types.h"
#include "graph/ge_local_context.h"
#include "common/dump/exception_dumper.h"
#undef private
#undef protected
#include "ge_graph_dsl/graph_dsl.h"
#include "depends/mmpa/src/mmpa_stub.h"

namespace ge {
class STEST_exception_dumper : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(STEST_exception_dumper, dump_exception_info) {
  const char_t * const kEnvRecordPath = "NPU_COLLECT_PATH";
  char_t npu_collect_path[MMPA_MAX_PATH] = "valid_path";
  mmSetEnv(kEnvRecordPath, &npu_collect_path[0U], MMPA_MAX_PATH);
  rtExceptionInfo exception_info = {1, 2, 3, 4, 5};
  std::vector<rtExceptionInfo> exception_infos = { exception_info };

  std::vector<uint8_t> input_stub(8);
  std::vector<uint8_t> output_stub(8);
  std::vector<void *> mock_arg{input_stub.data(), output_stub.data()};

  OpDescInfo op_desc_info;
  op_desc_info.op_name = "Save";
  op_desc_info.op_type = "Save";
  op_desc_info.task_id = 1;
  op_desc_info.stream_id = 2;
  op_desc_info.args = reinterpret_cast<uintptr_t>(mock_arg.data());
  op_desc_info.imply_type = static_cast<uint32_t>(domi::ImplyType::TVM);
  op_desc_info.input_format = {FORMAT_NCHW};
  op_desc_info.input_shape = {{1}};
  op_desc_info.input_data_type = {DT_FLOAT};
  op_desc_info.input_addrs = {nullptr};
  op_desc_info.input_size = {2};
  op_desc_info.output_format = {FORMAT_NCHW};
  op_desc_info.output_shape = {{1}};
  op_desc_info.output_data_type = {DT_FLOAT};
  op_desc_info.output_addrs = {nullptr};
  op_desc_info.output_size = {2};
  ExceptionDumper exception_dumper;
  exception_dumper.op_desc_info_ = { op_desc_info };
  exception_dumper.DumpExceptionInfo(exception_infos);
  unsetenv(kEnvRecordPath);
}

}  // namespace ge
