/**
 *
 * @file ffts_plus_ops_kernel_builder_unittest.cc
 *
 * @brief
 *
 * @version 1.0
 *
 */
#include <gtest/gtest.h>
#include <iostream>

#include <list>

#define private public
#define protected public
#include "common/sgt_slice_type.h"
#include "inc/ffts_log.h"
#include "inc/ffts_error_codes.h"
#include "inc/ffts_type.h"
#include "task_builder/fftsplus_ops_kernel_builder.h"
#include "task_builder/mode/auto/auto_thread_task_builder.h"
#include "task_builder/mode/manual/manual_thread_task_builder.h"
#include "graph/node.h"
#include "graph_builder_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "runtime/context.h"
#include "runtime/stream.h"
#include "runtime/rt_model.h"
#include "runtime/kernel.h"
#include "runtime/mem.h"
#include "../test_utils.h"

using namespace std;
using namespace ffts;
using namespace ge;
using FFTSPlusOpsKernelBuilderPtr = shared_ptr<FFTSPlusOpsKernelBuilder>;
const std::string kTaskRadio = "_task_ratio";
#define SET_SIZE 10000

class FFTSPlusOpsKernelBuilderSTest : public testing::Test{
 protected:
  static void SetUpTestCase() {
    cout << "FFTSPlusOpsKernelBuilderSTest SetUpTestCase" << endl;
  }
  static void TearDownTestCase() {
    cout << "FFTSPlusOpsKernelBuilderSTest TearDownTestCase" << endl;
  }

  // Some expensive resource shared by all tests.
  virtual void SetUp(){
    ffts_plus_ops_kernel_builder_ptr = make_shared<FFTSPlusOpsKernelBuilder>();
    ffts_plus_manual_mode_ptr = make_shared<ManualTheadTaskBuilder>();
    ffts_plus_auto_mode_ptr = make_shared<AutoTheadTaskBuilder>();
    std::map<std::string, std::string> options;
    ffts_plus_ops_kernel_builder_ptr->Initialize(options);
    _context = CreateContext();
  }
  virtual void TearDown(){
    cout << "a test Tear Down" << endl;
    ffts_plus_ops_kernel_builder_ptr->Finalize();

  }
  static RunContext CreateContext()
  {
    rtStream_t stream = (void *)0x123;
    rtModel_t model = (void *)0x123;

    RunContext context;
    context.model = model;
    context.stream = stream;
    context.dataMemSize = 101;
    context.dataMemBase = (uint8_t *) (intptr_t) 1000;
    context.weightMemSize = 200;
    context.weightMemBase = (uint8_t *) (intptr_t) 1101;
    context.weightsBuffer = Buffer(20);

    return context;
  }
 public:
  FFTSPlusOpsKernelBuilderPtr ffts_plus_ops_kernel_builder_ptr;
  ManualTheadTaskBuilderPtr ffts_plus_manual_mode_ptr;
  AutoTheadTaskBuilderPtr ffts_plus_auto_mode_ptr;
    RunContext _context;
};

void SetOpDecSize(NodePtr& node) {
  OpDesc::Vistor<GeTensorDesc> tensors = node->GetOpDesc()->GetAllInputsDesc();
  for (int i = 0; i < node->GetOpDesc()->GetAllInputsDesc().size(); i++) {
    ge::GeTensorDesc tensor = node->GetOpDesc()->GetAllInputsDesc().at(i);
    ge::TensorUtils::SetSize(tensor, SET_SIZE);
    node->GetOpDesc()->UpdateInputDesc(i, tensor);
  }
  OpDesc::Vistor<GeTensorDesc> tensorsOutput = node->GetOpDesc()->GetAllOutputsDesc();
  for (int i = 0; i < tensorsOutput.size(); i++) {
    ge::GeTensorDesc tensorOutput = tensorsOutput.at(i);
    ge::TensorUtils::SetSize(tensorOutput, SET_SIZE);
    node->GetOpDesc()->UpdateOutputDesc(i, tensorOutput);
  }
}

/*
 * Data -cast - netoutput
 */
ComputeGraphPtr BuildGraph_Readonly_Subgraph(const string &subraph_name){
  auto sub_builder = ut::ComputeGraphBuilder(subraph_name);
  auto data1 = sub_builder.AddNode("sdma1", "HcomAllReduce", 0,1);
  auto cast = sub_builder.AddNode("sdma2", "HcomAllReduce", 1,1);
  auto netoutput = sub_builder.AddNode("sdma3","HcomAllReduce", 1,1);
  AttrUtils::SetInt(data1->GetOpDesc(),ATTR_NAME_PARENT_NODE_INDEX, 1);
  AttrUtils::SetInt(netoutput->GetOpDesc(),ATTR_NAME_PARENT_NODE_INDEX,0);

  sub_builder.AddDataEdge(data1,0,cast,0);
  sub_builder.AddDataEdge(cast,0,netoutput,0);
  return sub_builder.GetGraph();
}

/*
 *      const - allreduce
 *            \ if
 *         insert identity
 */
ComputeGraphPtr BuildGraph_Readonly_ScopeWrite() {
  auto builder = ut::ComputeGraphBuilder("test");
  auto const1 = builder.AddNode("const1", CONSTANT, 0, 1);
  auto ctrl_const = builder.AddNode("ctrl_const", CONSTANT, 0, 1);
  auto allreduce = builder.AddNode("allreduce", "allreduce", 1, 1);
  auto if_node = builder.AddNode("if", "if", 1,0);

  builder.AddDataEdge(const1, 0, allreduce, 0);
  builder.AddDataEdge(const1, 0, if_node, 0);
  builder.AddControlEdge(ctrl_const, allreduce);

  auto root_graph = builder.GetGraph();
  string subgraph_name = "then_branch";
  ComputeGraphPtr then_branch_graph = BuildGraph_Readonly_Subgraph(subgraph_name);
  then_branch_graph->SetParentNode(if_node);
  then_branch_graph->SetParentGraph(root_graph);
  if_node->GetOpDesc()->AddSubgraphName(subgraph_name);
  if_node->GetOpDesc()->SetSubgraphInstanceName(0,subgraph_name);
  root_graph->AddSubgraph(subgraph_name, then_branch_graph);
  return root_graph;
}

static Status GenManualMixAICAIVCtxDef(NodePtr node) {
  auto op_desc = node->GetOpDesc();
  std::shared_ptr<domi::FftsPlusCtxDef> ffts_plus_def_ptr = make_shared<domi::FftsPlusCtxDef>();
  domi::FftsPlusMixAicAivCtxDef *mix_aic_aiv_ctx_def = ffts_plus_def_ptr->mutable_mix_aic_aiv_ctx();
  FFTS_CHECK_NOTNULL(mix_aic_aiv_ctx_def);

  uint32_t task_ratio;
  (void)ge::AttrUtils::GetInt(op_desc, kTaskRadio, task_ratio);

  mix_aic_aiv_ctx_def->set_prefetch_once_bitmap(0);
  mix_aic_aiv_ctx_def->set_prefetch_enable_bitmap(0);
  mix_aic_aiv_ctx_def->set_ns(1);
  mix_aic_aiv_ctx_def->set_atm(ffts::kManualMode);
  mix_aic_aiv_ctx_def->set_thread_dim(1);

  mix_aic_aiv_ctx_def->set_tail_block_ratio_n(task_ratio);
  mix_aic_aiv_ctx_def->set_non_tail_block_ratio_n(task_ratio);

  int32_t block_dim = 0;
  (void)ge::AttrUtils::GetInt(op_desc, ge::TVM_ATTR_NAME_BLOCKDIM, block_dim);
  mix_aic_aiv_ctx_def->set_tail_block_dim(static_cast<uint32_t>(block_dim));
  mix_aic_aiv_ctx_def->set_non_tail_block_dim(static_cast<uint32_t>(block_dim));

  // modeInArgsFirstField
  uint32_t mode = 0;
  (void)ge::AttrUtils::GetInt(op_desc, kModeInArgsFirstField, mode);
  // mode == 1 indicates we need reserve 8 Bytes for the args beginning
  if (mode == 1) {
    uint64_t modeArgs = 0;
    mix_aic_aiv_ctx_def->add_task_addr(modeArgs);
  }

  //input
  mix_aic_aiv_ctx_def->add_task_addr(222);
  //output
  mix_aic_aiv_ctx_def->add_task_addr(333);

  for (auto &prefix : kMixPrefixs) {
    string attr_key_kernel_name = prefix + op_desc->GetName() + kKernelName;
    string attr_kernel_name;
    (void)ge::AttrUtils::GetStr(op_desc, attr_key_kernel_name, attr_kernel_name);
    mix_aic_aiv_ctx_def->add_kernel_name(attr_kernel_name);
  }
  (void)ge::AttrUtils::SetInt(op_desc, ffts::kAttrAICoreCtxType, static_cast<int64_t>(ffts::TaskBuilderType::EN_TASK_TYPE_MIX_AIC_AIV));
  (void)op_desc->SetExtAttr(ffts::kAttrAICoreCtxDef, ffts_plus_def_ptr);
  return ffts::SUCCESS;
}


ComputeGraphPtr BuildGraph_Mix_Subgraph(const string &subraph_name) {
  auto builder = ut::ComputeGraphBuilder(subraph_name);
  auto lstm = builder.AddNode("LSTM", "LSTM", 2, 1, ge::FORMAT_NCHW, ge::DT_FLOAT, {2, 4, 4, 4});
  ge::AttrUtils::SetInt(lstm->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, static_cast<int>(domi::ImplyType::TVM));
  ge::AttrUtils::SetInt(lstm->GetOpDesc(), kModeInArgsFirstField, 1);
  ge::AttrUtils::SetInt(lstm->GetOpDesc(), kTaskRadio, 2);
  ge::AttrUtils::SetStr(lstm->GetOpDesc(), "_cube_vector_core_type", "MIX_AIC");
  (void)ge::AttrUtils::SetInt(lstm->GetOpDesc(), kAttrAICoreCtxType,
                              static_cast<int64_t>(ffts::TaskBuilderType::EN_TASK_TYPE_MIX_AIC_AIV));
  ge::AttrUtils::SetStr(lstm->GetOpDesc(), ge::TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF_MIX_AIC");
  ge::AttrUtils::SetInt(lstm->GetOpDesc(), kThreadScopeId, 0);
  (void)GenManualMixAICAIVCtxDef(lstm);
  ThreadSliceMapPtr threadSliceMap = std::make_shared<ThreadSliceMap>();
  lstm->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, threadSliceMap);
  SetOpDecSize(lstm);

  auto sub_graph = builder.GetGraph();
  return sub_graph;
}

/*
 *
 *            \
 *         insert identity
 */
ComputeGraphPtr BuildGraph_Mix_ScopeWrite() {
  auto builder = ut::ComputeGraphBuilder("test");
  auto sub_node = builder.AddNode("sub_node", "sub_node", 1, 0);
  auto root_graph = builder.GetGraph();
  string subgraph_name = "then_branch";
  ComputeGraphPtr then_branch_graph = BuildGraph_Mix_Subgraph(subgraph_name);
  then_branch_graph->SetParentNode(sub_node);
  then_branch_graph->SetParentGraph(root_graph);
  sub_node->GetOpDesc()->AddSubgraphName(subgraph_name);
  sub_node->GetOpDesc()->SetSubgraphInstanceName(0, subgraph_name);
  root_graph->AddSubgraph(subgraph_name, then_branch_graph);
  return root_graph;
}

static Status GenAutoAicAivCtxDef(NodePtr node) {
  std::shared_ptr<domi::FftsPlusCtxDef> ffts_plus_def_ptr = make_shared<domi::FftsPlusCtxDef>();
  domi::FftsPlusAicAivCtxDef *aic_aiv_ctx_def = ffts_plus_def_ptr->mutable_aic_aiv_ctx();
  FFTS_CHECK_NOTNULL(aic_aiv_ctx_def);

  aic_aiv_ctx_def->set_prefetch_once_bitmap(0);
  aic_aiv_ctx_def->set_prefetch_enable_bitmap(0);
  aic_aiv_ctx_def->set_aten(ffts::kAutoMode);
  aic_aiv_ctx_def->set_atm(ffts::kAutoMode);
  aic_aiv_ctx_def->set_thread_dim(4);

  int32_t block_dim = 1;
  aic_aiv_ctx_def->set_tail_block_dim(static_cast<uint32_t>(block_dim));
  aic_aiv_ctx_def->set_non_tail_block_dim(static_cast<uint32_t>(block_dim));

  //input
  aic_aiv_ctx_def->add_task_addr(111);
  aic_aiv_ctx_def->add_task_addr(222);

  //output
  aic_aiv_ctx_def->add_task_addr(333);

  //workspace
  aic_aiv_ctx_def->add_task_addr(444);

  auto op_desc = node->GetOpDesc();
  string attr_key_kernel_name = op_desc->GetName() + kKernelName;
  string attr_kernel_name;
  (void) ge::AttrUtils::GetStr(op_desc, attr_key_kernel_name, attr_kernel_name);
  aic_aiv_ctx_def->add_kernel_name(attr_kernel_name);
  (void)ge::AttrUtils::SetInt(op_desc, ffts::kAttrAICoreCtxType,
                              static_cast<int64_t>(ffts::TaskBuilderType::EN_TASK_TYPE_AIC_AIV_AUTO));
  (void)op_desc->SetExtAttr(ffts::kAttrAICoreCtxDef, ffts_plus_def_ptr);
  return ffts::SUCCESS;
}

/*
 *     data1     data2
 *       |        |
 *      add - - - -
 *       |
 *     relu
 *       |
 *    netoutput
 */
ComputeGraphPtr BuilGraph_Sgt_Subgraph_1(const string &subgraph_name, const uint32_t &thread_mode,
                                        const uint32_t &window_size, uint32_t slice_num = 4) {
  auto builder = ut::ComputeGraphBuilder(subgraph_name);
  auto data1 = builder.AddNode("data1", DATA, 0, 1);
  auto data2 = builder.AddNode("data2", DATA, 0, 1);
  auto add = builder.AddNode("add", "Add", 2, 1, FORMAT_NCHW, DT_FLOAT, {4, 4, 4, 4});
  auto relu = builder.AddNode("relu", RELU, 1, 1, FORMAT_NCHW, DT_FLOAT, {4, 4, 4, 4});
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 1);

  for (auto anchor : add->GetAllInDataAnchors()) {
    ge::AnchorUtils::SetStatus(anchor, ge::ANCHOR_DATA);
  }
  for (auto anchor : relu->GetAllInDataAnchors()) {
    ge::AnchorUtils::SetStatus(anchor, ge::ANCHOR_DATA);
  }
  GenAutoAicAivCtxDef(add);
  GenAutoAicAivCtxDef(relu);

  AttrUtils::SetInt(data1->GetOpDesc(),ATTR_NAME_PARENT_NODE_INDEX, 1);
  AttrUtils::SetInt(data2->GetOpDesc(),ATTR_NAME_PARENT_NODE_INDEX, 1);

  ThreadSliceMap thread_slice_map;
  thread_slice_map.thread_mode = thread_mode;
  thread_slice_map.parallel_window_size = window_size;
  thread_slice_map.slice_instance_num = slice_num;
  DimRange dim_rang;
  dim_rang.higher = 1;
  dim_rang.lower = 0;


  vector<vector<DimRange>> input_tensor_slice_vv;
  vector<DimRange> input_tensor_slice_v;
  input_tensor_slice_v.push_back(dim_rang);
  dim_rang.higher = 0;
  dim_rang.lower = 0;
  input_tensor_slice_v.push_back(dim_rang);
  input_tensor_slice_v.push_back(dim_rang);
  input_tensor_slice_v.push_back(dim_rang);
  input_tensor_slice_vv.push_back(input_tensor_slice_v);

  vector<vector<vector<DimRange>>> output_tensor_slice_vvv = {input_tensor_slice_vv, input_tensor_slice_vv,
                                                              input_tensor_slice_vv, input_tensor_slice_vv};
  input_tensor_slice_v.clear();
  dim_rang.higher = 1;
  dim_rang.lower = 1;
  input_tensor_slice_v.push_back(dim_rang);
  dim_rang.higher = 3;
  dim_rang.lower = 0;
  input_tensor_slice_v.push_back(dim_rang);
  input_tensor_slice_v.push_back(dim_rang);
  input_tensor_slice_v.push_back(dim_rang);
  input_tensor_slice_vv.push_back(input_tensor_slice_v);

  vector<vector<vector<DimRange>>> input_tensor_slice_vvv = {input_tensor_slice_vv, input_tensor_slice_vv,
                                                             input_tensor_slice_vv, input_tensor_slice_vv};
  thread_slice_map.input_tensor_slice = input_tensor_slice_vvv;
  thread_slice_map.output_tensor_slice = output_tensor_slice_vvv;
  ThreadSliceMapPtr thread_slice_map_ptr = std::make_shared<ThreadSliceMap>(thread_slice_map);
  thread_slice_map.input_tensor_slice = output_tensor_slice_vvv;
  ThreadSliceMapPtr thread_slice_map_ptr_relu = std::make_shared<ThreadSliceMap>(thread_slice_map);
  add->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, thread_slice_map_ptr);
  relu->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, thread_slice_map_ptr_relu);
  ge::AttrUtils::SetInt(add->GetOpDesc(), kThreadScopeId, 0);
  ge::AttrUtils::SetInt(relu->GetOpDesc(), kThreadScopeId, 0);

  int64_t ge_impl_type = static_cast<int>(domi::ImplyType::BUILDIN);
  ge::AttrUtils::SetInt(add->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, ge_impl_type);
  ge::AttrUtils::SetInt(relu->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, ge_impl_type);
  if (thread_mode == 1) {
    vector<string> thread_core_type = {"AIC", "AIC"};
    (void)ge::AttrUtils::SetListStr(add->GetOpDesc(), "_thread_cube_vector_core_type", thread_core_type);
    (void)ge::AttrUtils::SetListStr(relu->GetOpDesc(), "_thread_cube_vector_core_type", thread_core_type);
  } else {
    ge::AttrUtils::SetStr(add->GetOpDesc(), "_cube_vector_core_type", "AIC");
    ge::AttrUtils::SetStr(relu->GetOpDesc(), "_cube_vector_core_type", "AIC");
  }

  auto op_desc = netoutput->GetOpDesc();
  for (auto &tensor : op_desc->GetAllInputsDescPtr()) {
    ge::AttrUtils::SetInt(tensor, ATTR_NAME_PARENT_NODE_INDEX, 0);
  }

  builder.AddDataEdge(data1, 0, add, 0);
  builder.AddDataEdge(data2, 0, add, 1);
  builder.AddDataEdge(add, 0, relu, 0);
  builder.AddDataEdge(relu, 0, netoutput, 0);
  auto sub_graph = builder.GetGraph();
  return sub_graph;
}

/*
 *         data1     data2
 *           |        |
 *          add <- - -
 *           |  \
 *         relu  \
 *       /        \
 * netoutput2   netoutput1
 */
ComputeGraphPtr BuilGraph_Sgt_Subgraph_2(const string &subgraph_name, const bool &thread_mode,
                                       const uint32_t &window_size) {
  auto builder = ut::ComputeGraphBuilder(subgraph_name);
  auto data1 = builder.AddNode("data1", DATA, 0, 1);
  auto data2 = builder.AddNode("data2", DATA, 0, 1);
  auto add = builder.AddNode("add", "Add", 2, 1);
  auto relu = builder.AddNode("relu", RELU, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput1", NETOUTPUT, 1, 1);
  auto netoutput2 = builder.AddNode("netoutput2", NETOUTPUT, 1, 1);

  AttrUtils::SetInt(data1->GetOpDesc(),ATTR_NAME_PARENT_NODE_INDEX, 1);
  AttrUtils::SetInt(data2->GetOpDesc(),ATTR_NAME_PARENT_NODE_INDEX, 1);

  ThreadSliceMap thread_slice_map;
  thread_slice_map.thread_mode = thread_mode;
  thread_slice_map.parallel_window_size = window_size;
  ThreadSliceMapPtr thread_slice_map_ptr = std::make_shared<ThreadSliceMap>(thread_slice_map);
  add->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, thread_slice_map_ptr);
  relu->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, thread_slice_map_ptr);

  auto op_desc1 = netoutput1->GetOpDesc();
  for (auto &tensor1 : op_desc1->GetAllInputsDescPtr()) {
    ge::AttrUtils::SetInt(tensor1, ATTR_NAME_PARENT_NODE_INDEX, 0);
  }
  auto op_desc2 = netoutput2->GetOpDesc();
  for (auto &tensor2 : op_desc2->GetAllInputsDescPtr()) {
    ge::AttrUtils::SetInt(tensor2, ATTR_NAME_PARENT_NODE_INDEX, 0);
  }

  builder.AddDataEdge(data1, 0, add, 0);
  builder.AddDataEdge(data2, 0, add, 1);
  builder.AddDataEdge(add, 0, relu, 0);
  builder.AddDataEdge(add, 0, netoutput1, 0);
  builder.AddDataEdge(relu, 0, netoutput2, 0);
  auto sub_graph = builder.GetGraph();
  return sub_graph;
}

static Status GenManualAICAIVCtxDef(NodePtr node) {
  std::shared_ptr<domi::FftsPlusCtxDef> ffts_plus_def_ptr = make_shared<domi::FftsPlusCtxDef>();
  domi::FftsPlusAicAivCtxDef *aic_aiv_ctx_def = ffts_plus_def_ptr->mutable_aic_aiv_ctx();
  FFTS_CHECK_NOTNULL(aic_aiv_ctx_def);

  // cache managemet will do at GenerateDataTaskDef()
  aic_aiv_ctx_def->set_prefetch_once_bitmap(0);
  aic_aiv_ctx_def->set_prefetch_enable_bitmap(0);
  aic_aiv_ctx_def->set_atm(ffts::kManualMode);
  aic_aiv_ctx_def->set_thread_dim(1);

  int32_t block_dim = 1;
  //(void) ge::AttrUtils::GetInt(op_desc, ge::TVM_ATTR_NAME_BLOCKDIM, block_dim);
  aic_aiv_ctx_def->set_tail_block_dim(static_cast<uint32_t>(block_dim));
  aic_aiv_ctx_def->set_non_tail_block_dim(static_cast<uint32_t>(block_dim));

  //input
  aic_aiv_ctx_def->add_task_addr(111);
  aic_aiv_ctx_def->add_task_addr(222);

  //output
  aic_aiv_ctx_def->add_task_addr(333);

  //workspace
  aic_aiv_ctx_def->add_task_addr(444);

  auto op_desc = node->GetOpDesc();
  string attr_key_kernel_name = op_desc->GetName() + kKernelName;
  string attr_kernel_name;
  (void) ge::AttrUtils::GetStr(op_desc, attr_key_kernel_name, attr_kernel_name);
  aic_aiv_ctx_def->add_kernel_name(attr_kernel_name);
  (void)ge::AttrUtils::SetInt(op_desc, ffts::kAttrAICoreCtxType,
                              static_cast<int64_t>(ffts::TaskBuilderType::EN_TASK_TYPE_AIC_AIV));
  (void)op_desc->SetExtAttr(ffts::kAttrAICoreCtxDef, ffts_plus_def_ptr);
  return ffts::SUCCESS;
}

ComputeGraphPtr BuildGraph_SubGraph_Greater26(const string &subraph_name) {
  auto builder = ut::ComputeGraphBuilder(subraph_name);

  ThreadSliceMap thread_slice_map;
  thread_slice_map.thread_mode = 0;
  ThreadSliceMapPtr thread_slice_map_ptr = std::make_shared<ThreadSliceMap>(thread_slice_map);

  auto input = builder.AddNode("input", "input", 0, 30, ge::FORMAT_NCHW, ge::DT_FLOAT, {2, 4, 4, 4});
  (void)ge::AttrUtils::SetInt(input->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, static_cast<int>(domi::ImplyType::TVM));
  (void)ge::AttrUtils::SetStr(input->GetOpDesc(), "_cube_vector_core_type", "AIC");
  (void)ge::AttrUtils::SetInt(input->GetOpDesc(), kAttrAICoreCtxType,
                              static_cast<int64_t>(ffts::TaskBuilderType::EN_TASK_TYPE_AIC_AIV));
  (void)ge::AttrUtils::SetStr(input->GetOpDesc(), ge::TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF_AICUBE");
  (void)ge::AttrUtils::SetBool(input->GetOpDesc(), kTypeFFTSPlus, true);
  (void)ge::AttrUtils::SetInt(input->GetOpDesc(), kThreadScopeId, 1);
  input->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, thread_slice_map_ptr);
  GenManualAICAIVCtxDef(input);
  SetOpDecSize(input);

  auto output = builder.AddNode("output", "output", 30, 1, ge::FORMAT_NCHW, ge::DT_FLOAT, {2, 4, 4, 4});
  (void)ge::AttrUtils::SetInt(output->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, static_cast<int>(domi::ImplyType::TVM));
  (void)ge::AttrUtils::SetStr(output->GetOpDesc(), "_cube_vector_core_type", "AIC");
  (void)ge::AttrUtils::SetInt(output->GetOpDesc(), kAttrAICoreCtxType,
                              static_cast<int64_t>(ffts::TaskBuilderType::EN_TASK_TYPE_AIC_AIV));
  (void)ge::AttrUtils::SetStr(output->GetOpDesc(), ge::TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF_AICUBE");
  (void)ge::AttrUtils::SetBool(output->GetOpDesc(), kTypeFFTSPlus, true);
  (void)ge::AttrUtils::SetInt(output->GetOpDesc(), kThreadScopeId, 1);
  output->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, thread_slice_map_ptr);
  GenManualAICAIVCtxDef(output);
  SetOpDecSize(output);
  for (auto i = 0; i < 30; i++) {
    string node_name = "conv2d";
    node_name += to_string(i);
    auto conv2d = builder.AddNode(node_name, "conv2d", 1, 1, ge::FORMAT_NCHW, ge::DT_FLOAT, {2, 4, 4, 4});
    (void)ge::AttrUtils::SetInt(conv2d->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, static_cast<int>(domi::ImplyType::TVM));
    (void)ge::AttrUtils::SetStr(conv2d->GetOpDesc(), "_cube_vector_core_type", "AIC");
    (void)ge::AttrUtils::SetInt(conv2d->GetOpDesc(), kAttrAICoreCtxType,
                                static_cast<int64_t>(ffts::TaskBuilderType::EN_TASK_TYPE_AIC_AIV));
    (void)ge::AttrUtils::SetStr(conv2d->GetOpDesc(), ge::TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF_AICUBE");
    (void)ge::AttrUtils::SetBool(conv2d->GetOpDesc(), kTypeFFTSPlus, true);
    (void)ge::AttrUtils::SetInt(conv2d->GetOpDesc(), kThreadScopeId, 1);
    conv2d->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, thread_slice_map_ptr);
    SetOpDecSize(conv2d);
    GenManualAICAIVCtxDef(conv2d);
    builder.AddDataEdge(input, i, conv2d, 0);
    builder.AddDataEdge(conv2d, 0, output, i);
  }

  auto sub_graph = builder.GetGraph();
  return sub_graph;
}

ComputeGraphPtr BuildGraph_SubGraph_Greater60(const string &subraph_name) {
  auto builder = ut::ComputeGraphBuilder(subraph_name);

  ThreadSliceMap thread_slice_map;
  thread_slice_map.thread_mode = 0;
  ThreadSliceMapPtr thread_slice_map_ptr = std::make_shared<ThreadSliceMap>(thread_slice_map);

  auto input = builder.AddNode("input", "input", 0, 60, ge::FORMAT_NCHW, ge::DT_FLOAT, {2, 4, 4, 4});
  (void)ge::AttrUtils::SetInt(input->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, static_cast<int>(domi::ImplyType::TVM));
  (void)ge::AttrUtils::SetStr(input->GetOpDesc(), "_cube_vector_core_type", "AIV");
  (void)ge::AttrUtils::SetInt(input->GetOpDesc(), kAttrAICoreCtxType,
                              static_cast<int64_t>(ffts::TaskBuilderType::EN_TASK_TYPE_AIC_AIV));
  (void)ge::AttrUtils::SetStr(input->GetOpDesc(), ge::TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF_AIVEC");
  (void)ge::AttrUtils::SetBool(input->GetOpDesc(), kTypeFFTSPlus, true);
  (void)ge::AttrUtils::SetInt(input->GetOpDesc(), kThreadScopeId, 1);
  input->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, thread_slice_map_ptr);
  GenManualAICAIVCtxDef(input);
  SetOpDecSize(input);

  auto output = builder.AddNode("output", kTypePhonyConcat, 60, 1, ge::FORMAT_NCHW, ge::DT_FLOAT, {2, 4, 4, 4});
  (void)ge::AttrUtils::SetInt(output->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, static_cast<int>(domi::ImplyType::TVM));
  (void)ge::AttrUtils::SetStr(output->GetOpDesc(), "_cube_vector_core_type", "AIV");
  (void)ge::AttrUtils::SetInt(output->GetOpDesc(), kAttrAICoreCtxType,
                              static_cast<int64_t>(ffts::TaskBuilderType::EN_TASK_TYPE_AIC_AIV));
  (void)ge::AttrUtils::SetStr(output->GetOpDesc(), ge::TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF_AIVEC");
  (void)ge::AttrUtils::SetBool(output->GetOpDesc(), kTypeFFTSPlus, true);
  (void)ge::AttrUtils::SetInt(output->GetOpDesc(), kThreadScopeId, 1);
  output->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, thread_slice_map_ptr);
  GenManualAICAIVCtxDef(output);
  SetOpDecSize(output);

  auto output1 = builder.AddNode("output1", "output1", 1, 1, ge::FORMAT_NCHW, ge::DT_FLOAT, {2, 4, 4, 4});
  (void)ge::AttrUtils::SetInt(output1->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, static_cast<int>(domi::ImplyType::TVM));
  (void)ge::AttrUtils::SetStr(output1->GetOpDesc(), "_cube_vector_core_type", "AIV");
  (void)ge::AttrUtils::SetInt(output1->GetOpDesc(), kAttrAICoreCtxType,
                              static_cast<int64_t>(ffts::TaskBuilderType::EN_TASK_TYPE_AIC_AIV));
  (void)ge::AttrUtils::SetStr(output1->GetOpDesc(), ge::TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF_AIVEC");
  (void)ge::AttrUtils::SetBool(output1->GetOpDesc(), kTypeFFTSPlus, true);
  (void)ge::AttrUtils::SetInt(output1->GetOpDesc(), kThreadScopeId, 2);
  output1->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, thread_slice_map_ptr);
  GenManualAICAIVCtxDef(output1);
  SetOpDecSize(output1);

  for (auto i = 0; i < 60; i++) {
    string node_name = "conv2d";
    node_name += to_string(i);
    auto conv2d = builder.AddNode(node_name, "conv2d", 1, 1, ge::FORMAT_NCHW, ge::DT_FLOAT, {2, 4, 4, 4});
    (void)ge::AttrUtils::SetInt(conv2d->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, static_cast<int>(domi::ImplyType::TVM));
    (void)ge::AttrUtils::SetStr(conv2d->GetOpDesc(), "_cube_vector_core_type", "AIV");
    (void)ge::AttrUtils::SetInt(conv2d->GetOpDesc(), kAttrAICoreCtxType,
                                static_cast<int64_t>(ffts::TaskBuilderType::EN_TASK_TYPE_AIC_AIV));
    (void)ge::AttrUtils::SetStr(conv2d->GetOpDesc(), ge::TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF_AIVEC");
    (void)ge::AttrUtils::SetBool(conv2d->GetOpDesc(), kTypeFFTSPlus, true);
    (void)ge::AttrUtils::SetInt(conv2d->GetOpDesc(), kThreadScopeId, 1);
    conv2d->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, thread_slice_map_ptr);
    SetOpDecSize(conv2d);
    GenManualAICAIVCtxDef(conv2d);
    builder.AddDataEdge(input, i, conv2d, 0);
    builder.AddDataEdge(conv2d, 0, output, i);
  }
  builder.AddDataEdge(output, 0, output1, 0);
  auto sub_graph = builder.GetGraph();
  return sub_graph;
}

ComputeGraphPtr BuildGraph_Greater60() {
  auto builder = ut::ComputeGraphBuilder("test");
  auto sub_node = builder.AddNode("sub_node", "sub_node", 1, 0);
  auto root_graph = builder.GetGraph();
  string subgraph_name = "then_branch";
  ComputeGraphPtr then_branch_graph = BuildGraph_SubGraph_Greater60(subgraph_name);
  then_branch_graph->SetParentNode(sub_node);
  then_branch_graph->SetParentGraph(root_graph);
  sub_node->GetOpDesc()->AddSubgraphName(subgraph_name);
  sub_node->GetOpDesc()->SetSubgraphInstanceName(0, subgraph_name);
  root_graph->AddSubgraph(subgraph_name, then_branch_graph);
  return root_graph;
}

ComputeGraphPtr BuildGraph_AICPU_Subgraph(const string &subraph_name) {
  auto builder = ut::ComputeGraphBuilder(subraph_name);
  auto lstm = builder.AddNode("add", "Add", 2, 1);
  FftsPlusCtxDefPtr ctxDefPtr = std::make_shared<domi::FftsPlusCtxDef>();
  lstm->GetOpDesc()->SetExtAttr("_ffts_plus_aicpu_ctx_def", ctxDefPtr);
  ThreadSliceMapPtr threadSliceMap = std::make_shared<ThreadSliceMap>();
  lstm->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, threadSliceMap);
  ge::AttrUtils::SetInt(lstm->GetOpDesc(), kThreadScopeId, 0);

  auto sub_graph = builder.GetGraph();
  return sub_graph;
}
ComputeGraphPtr BuildGraph_HCCL_Subgraph(const string &subraph_name) {
  auto builder = ut::ComputeGraphBuilder(subraph_name);
  auto hccl_mode = builder.AddNode("HcomAllReduce", "HcomAllReduce", 1, 1);
  auto up_mode = builder.AddNode("upmode", "reduce", 1, 1, ge::FORMAT_NCHW, ge::DT_FLOAT, {2, 4, 4, 4});
  builder.AddDataEdge(up_mode, 0, hccl_mode, 0);
  std::vector<domi::FftsPlusCtxDef> hccl_sub_tasks;
  domi::FftsPlusCtxDef hccl_sub_task;
  hccl_sub_task.set_context_type(RT_CTX_TYPE_SDMA);
  hccl_sub_tasks.push_back(hccl_sub_task);
  hccl_sub_task.set_context_type(RT_CTX_TYPE_NOTIFY_WAIT);
  hccl_sub_tasks.push_back(hccl_sub_task);
  hccl_sub_task.set_context_type(RT_CTX_TYPE_WRITE_VALUE);
  hccl_sub_tasks.push_back(hccl_sub_task);

  hccl_mode->GetOpDesc()->SetExtAttr(kHcclSubTasks, hccl_sub_tasks);
  ThreadSliceMapPtr threadSliceMap = std::make_shared<ThreadSliceMap>();
  hccl_mode->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, threadSliceMap);
  up_mode->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, threadSliceMap);
  ge::AttrUtils::SetInt(hccl_mode->GetOpDesc(), kThreadScopeId, 2);
  ge::AttrUtils::SetInt(up_mode->GetOpDesc(), kThreadScopeId, 2);
  ge::AttrUtils::SetInt(up_mode->GetOpDesc(), ge::ATTR_NAME_IMPLY_TYPE, static_cast<int>(domi::ImplyType::TVM));
  AttrUtils::SetStr(up_mode->GetOpDesc(), "_cube_vector_core_type", "AIV");
  (void)ge::AttrUtils::SetInt(up_mode->GetOpDesc(), ffts::kAttrAICoreCtxType,
                              static_cast<int64_t>(ffts::TaskBuilderType::EN_TASK_TYPE_AIC_AIV));
  (void)GenManualAICAIVCtxDef(up_mode);
  (void)ge::AttrUtils::SetStr(up_mode->GetOpDesc(), ge::TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF_AIVEC");
  SetOpDecSize(up_mode);
  std::vector<std::vector<int64_t>> adjacency_list = {{1, 2}, {3}, {}};
  (void)ge::AttrUtils::SetListListInt(hccl_mode->GetOpDesc(), kAdjacencyList, adjacency_list);

  auto sub_graph = builder.GetGraph();
  return sub_graph;
}

ComputeGraphPtr BuildGraph_HCCL_Graph() {
  auto builder = ut::ComputeGraphBuilder("test");
  auto sub_node = builder.AddNode("sub_node", "sub_node", 1, 0);
  auto root_graph = builder.GetGraph();
  string subgraph_name = "then_branch";
  ComputeGraphPtr then_branch_graph = BuildGraph_HCCL_Subgraph(subgraph_name);
  then_branch_graph->SetParentNode(sub_node);
  then_branch_graph->SetParentGraph(root_graph);
  sub_node->GetOpDesc()->AddSubgraphName(subgraph_name);
  sub_node->GetOpDesc()->SetSubgraphInstanceName(0, subgraph_name);
  root_graph->AddSubgraph(subgraph_name, then_branch_graph);
  return root_graph;
}

ComputeGraphPtr BuildGraph_AICPU_ScopeWrite() {
  auto builder = ut::ComputeGraphBuilder("test");
  auto sub_node = builder.AddNode("sub_node", "sub_node", 1, 0);
  auto root_graph = builder.GetGraph();
  string subgraph_name = "then_branch";
  ComputeGraphPtr then_branch_graph = BuildGraph_AICPU_Subgraph(subgraph_name);
  then_branch_graph->SetParentNode(sub_node);
  then_branch_graph->SetParentGraph(root_graph);
  sub_node->GetOpDesc()->AddSubgraphName(subgraph_name);
  sub_node->GetOpDesc()->SetSubgraphInstanceName(0, subgraph_name);
  root_graph->AddSubgraph(subgraph_name, then_branch_graph);
  return root_graph;
}

TEST_F(FFTSPlusOpsKernelBuilderSTest, GenerateTask_SUCCESS)
{
  ComputeGraphPtr graph = BuildGraph_Readonly_ScopeWrite();
  auto ifnode = graph->FindNode("if");

  vector<domi::TaskDef> tasks;
  Status ret = ffts_plus_ops_kernel_builder_ptr->GenerateTask(*ifnode, _context, tasks);

  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FFTSPlusOpsKernelBuilderSTest, GenerateTask_Greater60_SUCCESS)
{
  ComputeGraphPtr graph = BuildGraph_Greater60();
  auto sub_node = graph->FindNode("sub_node");
  vector<domi::TaskDef> tasks;
  Status ret = ffts_plus_ops_kernel_builder_ptr->GenerateTask(*sub_node, _context, tasks);
  domi::FftsPlusTaskDef *ffts_plus_task_def = tasks[0].mutable_ffts_plus_task();
  EXPECT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 64);
}


TEST_F(FFTSPlusOpsKernelBuilderSTest, Mix_GenerateTask_SUCCESS)
{
  ComputeGraphPtr graph = BuildGraph_Mix_ScopeWrite();
  cout << "========================MIX AIC/AIV GENTASK BEGIN========================" <<
  endl;
  auto sub_node = graph->FindNode("sub_node");

  if (sub_node == nullptr) {
  cout << "[ERROR] FE:sub node is nullptr";
  }
  vector <domi::TaskDef> tasks;
  Status ret = ffts_plus_ops_kernel_builder_ptr->GenerateTask(*sub_node, _context, tasks);

  EXPECT_EQ(ffts::SUCCESS, ret
  );
}


TEST_F(FFTSPlusOpsKernelBuilderSTest, Case_Gen_AutoThread_Context_Id)
{
  std::vector<ge::NodePtr> sub_graph_nodes;
  uint32_t thread_mode = true;
  uint32_t window_size = 4;
  ComputeGraphPtr sgt_graph = BuilGraph_Sgt_Subgraph_1("sgt_graph", thread_mode, window_size);
  uint64_t ready_context_num = 0;
  uint64_t total_context_number = 0;
  Status ret = ffts_plus_auto_mode_ptr->GenFftsPlusContextId(*sgt_graph, sub_graph_nodes, ready_context_num,
                                                                      total_context_number);
  // check result
  int total_count = 0;
  int flag_attr_count = 0;
  for (const auto &node : sub_graph_nodes) {
    total_count++;
    if (node->GetName() == "add") {
      uint32_t in_label_ctx_id = 1;
      vector<uint32_t> at_start_ctx_id_list;
      vector<uint32_t> context_id_list;
      if (ge::AttrUtils::GetInt(node->GetOpDesc(), kAutoInlabelCtxId, in_label_ctx_id) &&
          ge::AttrUtils::GetListInt(node->GetOpDesc(), kAutoAtStartCtxIdList, at_start_ctx_id_list) &&
          ge::AttrUtils::GetListInt(node->GetOpDesc(), kAutoCtxIdList, context_id_list)) {
        flag_attr_count++;
      }
    } else if (node->GetName() == "relu") {
      uint32_t at_end_pre_cnt = 0;
      uint32_t out_label_ctx_id = 0;
      vector<uint32_t> at_end_ctx_id_list;
      vector<uint32_t> context_id_list;
      if (ge::AttrUtils::GetInt(node->GetOpDesc(), kAutoOutlabelCtxId, out_label_ctx_id) &&
          ge::AttrUtils::GetListInt(node->GetOpDesc(), kAutoAtEndCtxIdList, at_end_ctx_id_list) &&
          ge::AttrUtils::GetInt(node->GetOpDesc(), kAutoAtEndPreCnt, at_end_pre_cnt) &&
          ge::AttrUtils::GetListInt(node->GetOpDesc(), kAutoCtxIdList, context_id_list)) {
        flag_attr_count++;
      }
    }
  }
  EXPECT_EQ(2, total_count);
  EXPECT_EQ(2, flag_attr_count);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FFTSPlusOpsKernelBuilderSTest, Case_Gen_AutoThread_AtEnd_PreCnt)
{
  std::vector<ge::NodePtr> sub_graph_nodes;
  bool thread_mode = 1;
  uint32_t window_size = 4;
  ComputeGraphPtr sgt_graph = BuilGraph_Sgt_Subgraph_2("sgt_graph", thread_mode, window_size);
  uint64_t ready_context_num = 0;
  uint64_t total_context_number = 0;
  Status ret = ffts_plus_auto_mode_ptr->GenFftsPlusContextId(*sgt_graph, sub_graph_nodes, ready_context_num,
                                                                      total_context_number);
  // check result
  bool status_pre_cnt = false;
  for (const auto &node : sub_graph_nodes) {
    if (node->GetName() == "relu") {
      uint32_t at_end_pre_cnt = 0;
      (void)ge::AttrUtils::GetInt(node->GetOpDesc(), kAutoAtEndPreCnt, at_end_pre_cnt);
      if (at_end_pre_cnt == 2) {
        status_pre_cnt = true;
      }
    }
  }
  EXPECT_EQ(true, status_pre_cnt);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FFTSPlusOpsKernelBuilderSTest, Case_Gen_Sub_Graph_Task_Def)
{
  std::vector<ge::NodePtr> sub_graph_nodes;
  uint32_t thread_mode = 1;
  uint32_t window_size = 4;
  ComputeGraphPtr sgt_graph = BuilGraph_Sgt_Subgraph_1("sgt_graph", thread_mode, window_size);
  uint64_t ready_context_num = 0;
  uint64_t total_context_number = 0;
  ffts_plus_auto_mode_ptr->Initialize();
  Status ret = ffts_plus_auto_mode_ptr->GenFftsPlusContextId(*sgt_graph, sub_graph_nodes, ready_context_num,
                                                                      total_context_number);

  RunContext context = CreateContext();
  domi::TaskDef task_def;
  std::cout << std::endl << std::endl << "-----" << std::endl << std::endl;
  ret = ffts_plus_auto_mode_ptr->GenSubGraphTaskDef(sub_graph_nodes, task_def);
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  uint64_t gen_ctx_num = ffts_plus_task_def->ffts_plus_ctx_size();

  EXPECT_EQ(18, gen_ctx_num);
  EXPECT_EQ(ffts::SUCCESS, ret);
}

/*
 *         data1
 *           |
 *       labelswitch
 *           |
 *         labelset
 *           |
 *        identity
 *           |
 *        output
 */
ComputeGraphPtr BuildGraph_RuntimeOp_Subgraph(const string &subraph_name) {
  auto builder = ut::ComputeGraphBuilder(subraph_name);
  auto data = builder.AddNode("data", DATA, 0, 1);
  auto labelswitch = builder.AddNode("labelswitch", "LabelSwitchByIndex", 1, 1);
  auto labelset = builder.AddNode("labelset", "LabelSet", 1, 1);
  auto identity = builder.AddNode("identity", "Identity", 1, 1);
  auto output = builder.AddNode("output", NETOUTPUT, 1, 1);
  FftsPlusCtxDefPtr ctxDefPtr = std::make_shared<domi::FftsPlusCtxDef>();
  labelswitch->GetOpDesc()->SetExtAttr("FFTS_PLUS_TASK_DEF", ctxDefPtr);
  labelset->GetOpDesc()->SetExtAttr("FFTS_PLUS_TASK_DEF", ctxDefPtr);
  labelswitch->GetOpDesc()->SetExtAttr("FFTS_PLUS_TASK_DEF", ctxDefPtr);
  identity->GetOpDesc()->SetExtAttr("FFTS_PLUS_TASK_DEF", ctxDefPtr);
  ThreadSliceMapPtr threadSliceMap = std::make_shared<ThreadSliceMap>();
  labelswitch->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, threadSliceMap);
  ge::AttrUtils::SetInt(labelswitch->GetOpDesc(), kThreadScopeId, 0);
  labelset->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, threadSliceMap);
  ge::AttrUtils::SetInt(labelset->GetOpDesc(), kThreadScopeId, 0);
  identity->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, threadSliceMap);
  ge::AttrUtils::SetInt(identity->GetOpDesc(), kThreadScopeId, 0);

 
  builder.AddDataEdge(data, 0, labelswitch, 0);
  builder.AddDataEdge(labelswitch, 0, labelswitch, 0);
  builder.AddDataEdge(labelswitch, 0, labelset, 0);
  builder.AddDataEdge(labelset, 0, identity, 0);
  builder.AddDataEdge(identity, 0, output, 0);

  auto sub_graph = builder.GetGraph();
  return sub_graph;
}

ComputeGraphPtr BuildGraph_RuntimeOp_ScopeWrite() {
  auto builder = ut::ComputeGraphBuilder("rts_root_graph");
  auto sub_node = builder.AddNode("sub_node", "sub_node", 1, 0);
  auto root_graph = builder.GetGraph();
  string subgraph_name = "func_op_branch";
  ComputeGraphPtr func_op_branch_graph = BuildGraph_RuntimeOp_Subgraph(subgraph_name);
  func_op_branch_graph->SetParentNode(sub_node);
  func_op_branch_graph->SetParentGraph(root_graph);
  sub_node->GetOpDesc()->AddSubgraphName(subgraph_name);
  sub_node->GetOpDesc()->SetSubgraphInstanceName(0, subgraph_name);
  root_graph->AddSubgraph(subgraph_name, func_op_branch_graph);
  return root_graph;
}

TEST_F(FFTSPlusOpsKernelBuilderSTest, RTSOP_GenerateTask_SUCCESS)
{
  ComputeGraphPtr graph = BuildGraph_RuntimeOp_ScopeWrite();
  cout << "========================RTSOP GENTASK BEGIN========================" << endl;
  auto sub_node = graph->FindNode("sub_node");

  if (sub_node == nullptr) {
    cout << "[ERROR] FE:sub node is nullptr";
  }
  RunContext context = CreateContext();
  vector<domi::TaskDef> tasks;
  Status ret = ffts_plus_ops_kernel_builder_ptr->GenerateTask(*sub_node, context, tasks);

  EXPECT_EQ(ffts::SUCCESS, ret);
}
TEST_F(FFTSPlusOpsKernelBuilderSTest, AICPU_GenerateTask_SUCCESS)
{
  ComputeGraphPtr graph = BuildGraph_AICPU_ScopeWrite();
  cout << "========================aicpu GENTASK BEGIN========================" << endl;
  auto sub_node = graph->FindNode("sub_node");

  if (sub_node == nullptr) {
    cout << "[ERROR] FE:sub node is nullptr";
  }
  RunContext context = CreateContext();
  vector<domi::TaskDef> tasks;
  Status ret = ffts_plus_ops_kernel_builder_ptr->GenerateTask(*sub_node, context, tasks);

  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FFTSPlusOpsKernelBuilderSTest, HCCL_GenerateTask_SUCCESS)
{
  ComputeGraphPtr graph = BuildGraph_HCCL_Graph();
  cout << "========================hccl GENTASK BEGIN========================" << endl;
  auto sub_node = graph->FindNode("sub_node");
  if (sub_node == nullptr) {
  cout << "[ERROR] FE:sub node is nullptr";
  }
  RunContext context = CreateContext();
  vector<domi::TaskDef> tasks;
  Status ret = ffts_plus_ops_kernel_builder_ptr->GenerateTask(*sub_node, context, tasks);

  EXPECT_EQ(ffts::SUCCESS, ret);
}

TEST_F(FFTSPlusOpsKernelBuilderSTest, AutoFillContextSuccList)
{
  auto builder = ut::ComputeGraphBuilder("test");
  auto input = builder.AddNode("test", "test", 0, 1);
  auto out_node = builder.AddNode("test1", "NetOutput", 1, 0);
  builder.AddDataEdge(input, 0, out_node, 0);
  vector<uint32_t> context_id_list = {3, 4};
  ge::AttrUtils::SetListInt(out_node->GetOpDesc(), kAutoCtxIdList, context_id_list);
  domi::FftsPlusTaskDef taskdef;
  domi::FftsPlusCtxDef *ffts_plus_ctx_def = taskdef.add_ffts_plus_ctx();
  ffts_plus_ctx_def->set_context_type(RT_CTX_TYPE_NOTIFY_RECORD);
  ffts_plus_ctx_def->set_context_id(0);
  ffts_plus_ctx_def = taskdef.add_ffts_plus_ctx();
  ffts_plus_ctx_def->set_context_type(RT_CTX_TYPE_SDMA);
  ffts_plus_ctx_def->set_context_id(1);
  ffts_plus_ctx_def = taskdef.add_ffts_plus_ctx();
  ffts_plus_ctx_def->set_context_type(RT_CTX_TYPE_NOTIFY_WAIT);
  ffts_plus_ctx_def->set_context_id(2);
  ffts_plus_ctx_def = taskdef.add_ffts_plus_ctx();
  ffts_plus_ctx_def->set_context_type(RT_CTX_TYPE_WRITE_VALUE);
  ffts_plus_ctx_def->set_context_id(3);
  FFTSPlusTaskBuilderPtr task_builder = nullptr;
  vector<uint32_t> context_list;
  context_list.push_back(0);
  context_list.push_back(1);
  context_list.push_back(2);
  context_list.push_back(3);
  Status ret = ffts_plus_auto_mode_ptr->FillContextSuccList(input, &taskdef, task_builder, context_list,
                                                            context_list);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(FFTSPlusOpsKernelBuilderSTest, MIX_L2_GenerateTask_SUCCESS)
{
  RunContext context = CreateContext();
  vector<domi::TaskDef> tasks;
  auto builder = ut::ComputeGraphBuilder("test");
  auto sub_node = builder.AddNode("mix_node", "mix_node", 1, 0);
  (void)ge::AttrUtils::SetStr(sub_node->GetOpDesc(), ffts::ATTR_NAME_ALIAS_ENGINE_NAME, "ffts_plus");
  FftsPlusCtxDefPtr ffts_plus_ctx_def = nullptr;
  ffts_plus_ctx_def = std::make_shared<domi::FftsPlusCtxDef>();
  domi::FftsPlusMixAicAivCtxDef *mix_l2_ctx = ffts_plus_ctx_def->mutable_mix_aic_aiv_ctx();
  mix_l2_ctx->set_ns(1);
  mix_l2_ctx->set_atm(1);
  mix_l2_ctx->set_thread_dim(1);
  mix_l2_ctx->set_tail_block_ratio_n(2);
  mix_l2_ctx->set_non_tail_block_ratio_n(2);
  mix_l2_ctx->add_task_addr(0x1114);
  mix_l2_ctx->add_task_addr(0x1111);
  mix_l2_ctx->add_task_addr(0x1112);
  mix_l2_ctx->add_kernel_name("mix1");
  mix_l2_ctx->add_kernel_name("mix2");
  (void)sub_node->GetOpDesc()->SetExtAttr(kAttrAICoreCtxDef, ffts_plus_ctx_def);
  (void) ge::AttrUtils::SetInt(sub_node->GetOpDesc(), kModeInArgsFirstField, 1);
  Status ret = ffts_plus_ops_kernel_builder_ptr->GenerateTask(*sub_node, context, tasks);
  EXPECT_EQ(fe::SUCCESS, ret);
}