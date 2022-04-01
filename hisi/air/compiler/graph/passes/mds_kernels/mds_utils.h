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
#ifndef MAIN_GRAPHENGINE_GE_GRAPH_PASSES_MDS_KERNELS_MDS_UTILS_H_
#define MAIN_GRAPHENGINE_GE_GRAPH_PASSES_MDS_KERNELS_MDS_UTILS_H_

#include "common/op/ge_op_utils.h"
#include "graph/utils/type_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "ge/ge_api_types.h"
#include "graph/compute_graph.h"
#include "graph/shape_refiner.h"
#include "framework/common/types.h"
#include "graph/utils/op_desc_utils.h"
#include "common/plugin/ge_util.h"
#include "common/formats/utils/formats_trans_utils.h"
#include "runtime/dev.h"
#include "runtime/base.h"
#include "runtime/context.h"

#define REQUIRE(cond, ...)                       \
  do {                                           \
    if (!(cond)) {                               \
      REPORT_INNER_ERROR("E19999", __VA_ARGS__); \
      GELOGE(FAILED, "[MDS]" __VA_ARGS__);       \
      return FAILED;                             \
    }                                            \
  } while (false)

#define REQUIRE_AND_DUMP(cond, ...)              \
  do {                                           \
    if (!(cond)) {                               \
      GE_DUMP(compute_graph_, "MDS_FAILED");     \
      REPORT_INNER_ERROR("E19999", __VA_ARGS__); \
      GELOGE(FAILED, "[MDS]" __VA_ARGS__);       \
      return FAILED;                             \
    }                                            \
  } while (false)

#define MDS_REQUIRE_NOT_NULL(cond, ...) REQUIRE(((cond) != nullptr), __VA_ARGS__)
#define MDS_REQUIRE_SUCCESS(cond, ...) REQUIRE(((cond) == SUCCESS), __VA_ARGS__)
#define MDS_REQUIRE_SUCCESS_AND_DUMP(cond, ...) REQUIRE_AND_DUMP(((cond) == SUCCESS), __VA_ARGS__)
#define MDS_REQUIRE_GRAPH_SUCCESS(cond, ...) REQUIRE(((cond) == GRAPH_SUCCESS), __VA_ARGS__)
namespace ge {
// Invalid location index
enum class NCutIndex { kNLocation0 = 0, kNLocation1 = 1, kNLocation2 = 2, kNLocation3 = 3, kNInvalidLocation = -1 };
enum class HCutIndex { kHLocation0 = 0, kHLocation1 = 1, kHLocation2 = 2, kHLocation3 = 3, kHInvalidLocation = -1 };
// default die number
constexpr int64_t kDeployNumber = 2;
enum class CutType { kNoCut = 0, kCutN, kCutH, kDynamicCutN, kDynamicCutH, kDynamicCutAll };
constexpr char_t const *kDeviceChipType = "chip";
constexpr char_t const *kDeviceDieType = "die";
constexpr char_t const *kDieDeviceTypeValue = "MultiMode";
using GraphInputs = std::vector<GeTensorPtr>;

class MdsUtils {
 public:
  // Parse the configuration file and determine whether to enable MDS based on the value of device_type.
  static bool IsMDSNeeded();
  static bool IsGradNode(const NodePtr &node);
  static int64_t GetNLocation(const Format fmt);
  static int64_t GetHLocation(const Format fmt);

  static int64_t GetIndexByFormat(const ConstGeTensorDescPtr &ge_tensor_desc, const CutType type);
  static bool IsDistributedDeploySupported(const GeTensorDescPtr &ge_tensor_desc, const CutType type,
                                           const bool check_divisible = false);
  static Status SetAttrForHcomNode(const OpDescPtr &hcom_op, const int64_t fission_factor,
                                   const std::string &group_name = "");
  // Sets the information, notifies the number of threads to be started during the
  // loading phase, the device on which each thread should run, and constructs different input data on each device.
  static Status SetDeployInfo(const ComputeGraphPtr &compute_graph, const std::multimap<int64_t, GraphInputs> &deploys,
                              const std::string &device_type = kDieDeviceTypeValue);
  // Get cut policy for whole graph
  static CutType TryGetGraphCutType(const ComputeGraphPtr &compute_graph);

  static Status DataGather(const OutDataAnchorPtr &src, const InDataAnchorPtr &dst);
  static Status DataReduce(const OutDataAnchorPtr &src, const InDataAnchorPtr &dst);
  static Status DataSlice(const OutDataAnchorPtr &src, const InDataAnchorPtr &dst, const CutType cut_type,
                          NodePtr &input_node);
  static Status InferShapeAndType(const NodePtr &node);
  static Status SetDevice(const int32_t device);
  static Status CtxCreate(rtContext_t *const ctx, const uint32_t  flags, const int32_t device);

 private:
  static NodePtr AddDynamicInputOutputNode(const ComputeGraphPtr &graph, const string &type, const string &node_name,
                                           const size_t input_num, const size_t output_num);
  static NodePtr AddSingleInputOutputNode(const ComputeGraphPtr &graph, const string &name, const string &type,
                                          const GeTensorDesc &tensor = GeTensorDesc());
  static Status ConstructReduceNode(const ComputeGraphPtr &src_graph, const OutDataAnchorPtr &src,
                                    const InDataAnchorPtr &dst, NodePtr &reduce_node);
  static Status ConstructSliceNode(const ComputeGraphPtr &src_graph, const ConstGeTensorDescPtr &tensor_desc,
                                   const NodePtr &input_node, const int8_t cut_index, NodePtr &slice_node);
  static bool NeedInsertHcomAllReduce(const NodePtr &src_node, NodePtr &allreduce_node);
  static NodePtr AddConstNodeToGraph(const GeTensorPtr &tensor, const ComputeGraphPtr &graph);
  static Status ConstructTensorDescWithData(const GeTensorDesc &out_desc,
                                     const vector<int64_t> &data,
                                     vector<GeTensorPtr> &v_output,
                                     const bool scalar_output = false);
  template<typename T>
  static Status ConstructTensorDescWithData(const GeTensorDesc &out_desc,
                                     T *const buf,
                                     const uint32_t len,
                                     vector<GeTensorPtr> &v_output,
                                     const bool scalar_output);
};
}  // namespace ge
#endif  // MAIN_GRAPHENGINE_GE_GRAPH_PASSES_MDS_KERNELS_MDS_UTILS_H_
