/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_DAVINCI_MODEL_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_DAVINCI_MODEL_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "framework/common/ge_types.h"
#include "framework/common/helper/model_helper.h"
#include "framework/common/helper/om_file_helper.h"
#include "common/opskernel/ge_task_info.h"
#include "common/properties_manager.h"
#include "common/dump/exception_dumper.h"
#include "common/dump/opdebug_register.h"
#include "framework/common/util.h"
#include "graph/load/model_manager/aipp_utils.h"
#include "common/dump/data_dumper.h"
#include "graph/load/model_manager/data_inputer.h"
#include "graph/load/model_manager/tbe_kernel_handle.h"
#include "graph/load/model_manager/zero_copy_offset.h"
#include "graph/model.h"
#include "graph/node.h"
#include "graph/op_desc.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"
#include "mmpa/mmpa_api.h"
#include "proto/task.pb.h"
#include "graph/load/model_manager/task_info/task_info_factory.h"
#include "common/local_context.h"
#include "graph/load/model_manager/aicpu_resources.h"
#include "graph/load/model_manager/cpu_queue_schedule.h"
#include "toolchain/prof_common.h"

namespace ge {
enum class ModelProcStage {
  MODEL_LOAD_START = 1,
  MODEL_LOAD_END,
  MODEL_PRE_PROC_START,
  MODEL_PRE_PROC_END,
  MODEL_INFER_START,
  MODEL_INFER_END,
  MODEL_AFTER_PROC_START,
  MODEL_AFTER_PROC_END,
  MODEL_PROC_INVALID,
};

// For super kernel
struct SuperKernelTaskInfo {
  uint32_t last_block_dim;
  uint32_t last_args_size;
  uint32_t last_task_id;
  uint32_t last_stream_id;
  void *last_stream;
  void *last_sm_desc;
  std::vector<const void *> kernel_list;
  std::vector<void *> arg_list;
  std::vector<uint32_t> dump_flag_list;
  std::vector<OpDescPtr> op_desc_list;
  std::vector<uintptr_t> dump_args_list;
  uint32_t last_dump_flag;
  int64_t last_group_key;
  uintptr_t last_dump_args;
  OpDescPtr last_op;
};

struct SktTaskInfo {
  void *stream;
  void *sm_desc;
  const void *kernel;
  void *args;
  uint32_t args_size;
  uint32_t block_dim;
  int64_t group_key;
  uint32_t dump_flag;
  uintptr_t dump_args;
};

struct ProfileInfo {
  FusionOpInfo fusion_info;
  std::vector<MsprofGeProfFusionData> prof_fusion_data_lst;
};

enum class ExecuteMode {
  INITIALIZATION,
  SYNCHRONIZATION,
  ASYNCHRONIZATION,
};

// comments
class DavinciModel {
 public:
  ///
  /// @ingroup ge
  /// @brief DavinciModel constructor
  /// @author
  ///
  DavinciModel(const int32_t priority, const std::shared_ptr<ModelListener> &listener);

  ///
  /// @ingroup ge
  /// @brief DavinciModel desctructor, free Parse and Init resources
  /// @author
  ///
  ~DavinciModel();

  ///
  /// @ingroup ge
  /// @brief apply model to model_def_
  ///
  void Assign(const GeModelPtr &ge_model);

  ///
  /// @ingroup ge
  /// @brief check model whther it has input or output queue.
  /// @return true : no input or output queue, false : has input or output queue.
  ///
  bool CheckModelNoInputAndOutput() const;

  ///
  /// @ingroup ge
  /// @brief DavinciModel initialization, including Stream, ccHandle, Event, DataInputer, etc
  /// @return execute result
  /// @author
  ///
  Status Init(const uintptr_t mem_ptr = 0U, const size_t mem_size = 0U,
              const uintptr_t weight_ptr = 0U, const size_t weight_size = 0U);

  ///
  /// @ingroup ge
  /// @brief ACL case, Load task list with queue.
  /// @param [in] input_que_ids: input queue ids from user, nums equal Data Op.
  /// @param [in] output_que_ids: input queue ids from user, nums equal NetOutput Op.
  /// @return: 0 for success / others for fail
  ///
  Status SetQueIds(const std::vector<uint32_t> &input_queue_ids, const std::vector<uint32_t> &output_queue_ids);

  ///
  /// @ingroup ge
  /// @brief Get DataInputer
  /// @return model ID
  ///
  uint32_t Id() const { return model_id_; }

  ///
  /// @ingroup ge
  /// @brief Get DataInputer
  /// @return model ID
  ///
  void SetId(const uint32_t model_id);

  ///
  /// @ingroup ge
  /// @brief Get SubModelId
  /// @return sub model ID
  ///
  uint32_t SubModelId() const { return sub_model_id_; }

  ///
  /// @ingroup ge
  /// @brief Get SubModelId
  /// @return sub model ID
  ///
  void SetSubModelId(const uint32_t sub_model_id) { sub_model_id_ = sub_model_id; }

  void SetRunContext(const OmeContext &context) { run_context_ = context; }

  void Run();

  void *GetOverflowAddr() const {
    return globalworkspace_overflow_addr_;
  }

  void SetOverflowAddr(void *const overflow_addr) {
    globalworkspace_overflow_addr_ = overflow_addr;
  }
  ///
  /// @ingroup ge
  /// @brief NnExecute
  /// @param [in] stream   execute stream
  /// @param [in] async_mode  is asynchronize mode.
  /// @param [in] input_data  model input data
  /// @param [out] output_data  model output data
  ///
  Status NnExecute(const rtStream_t stream, const bool async_mode,
                   const InputData &input_data, OutputData &output_data);

  ///
  /// @ingroup ge
  /// @brief get Push Data to Queue
  /// @return 0 for success / others for fail
  ///
  Status Push(const std::shared_ptr<InputDataWrapper> &data) {
    return data_inputer_.Push(data);
  }

  uint32_t GetDataInputerSize() {
    return data_inputer_.Size();
  }

  // get model priority
  int32_t Priority() const { return priority_; }

  // get total mem size
  size_t TotalMemSize() const { return runtime_param_.mem_size; }

  ///
  /// @ingroup ge
  /// @brief Get total useful size, in known subgraph, no need to allocate zero copy memory during initialization.
  /// @param [in] total_useful_size: total mem size - zero copy size.
  /// @return Status
  ///
  Status GetTotalMemSizeExcludeZeroCopy(int64_t &total_useful_size);

  size_t TotalVarMemSize() const { return runtime_param_.var_size; }

  // get base memory address
  uintptr_t FeatureMapBase() const { return mem_base_; }

  // get Event list
  const std::vector<rtEvent_t> &GetEventList() const { return event_list_; }

  const std::vector<rtStream_t> &GetStreamList() const { return stream_list_; }

  const std::vector<rtLabel_t> &GetLabelList() const { return label_list_; }

  uint64_t GetAllStreamNum() const { return stream_list_.size() + all_hccl_stream_list_.size(); }

  Status GetLabelGotoAddr(const uint32_t label_index, const rtMemType_t mem_type, void *&arg_addr, uint32_t &arg_size);

  Status DestroyThread();

  // get Op
  OpDescPtr GetOpByIndex(const uint32_t op_index) const {
    const auto it = op_list_.find(static_cast<int64_t>(op_index));
    if (it == op_list_.end()) {
      return nullptr;
    }
    return it->second;
  }

  void SetGlobalStep(const uintptr_t step_addr, const uint64_t step_size);
  uintptr_t GetGlobalStep() const { return global_step_addr_; }

  // get updated task info list
  const std::vector<TaskInfoPtr> &GetTaskList() const { return task_list_; }

  // Read from KernelTaskInfo.
  const SuperKernelTaskInfo &GetSuperKernelTaskInfo() const { return skt_info_; }

  void SuperKernelSaveTask(const OpDescPtr &op_desc, const SktTaskInfo &skt_task_info);

  void SuperKernelUpdateTaskId(uint32_t &skt_task_id);

  void SuperKernelFinalize(void *const sm_desc, const int64_t group_key);

  rtModel_t GetRtModelHandle() const { return rt_model_handle_; }

  uint64_t GetRtBaseAddr() const { return runtime_param_.logic_mem_base; }

  uint32_t GetFlowctrlIndex(const uint32_t op_index);

  void PushHcclStream(const rtStream_t hccl_stream);

  CustAICPUKernelPtr GetCustAICPUKernel(const OpDescPtr &op_desc) const;

  ///
  /// @ingroup ge
  /// @brief For TVM Op, avoid Addr Reuse.
  /// @return void*
  ///
  const char_t *GetRegisterStub(const std::string &binfile, const std::string &session_graph_id = "");

  ///
  /// @ingroup ge
  /// @brief get model input and output desc info
  /// @param [out] input_shape  model input size
  /// @param [out] output_shape model output size
  /// @return execute result
  ///
  Status GetInputOutputDescInfo(std::vector<InputOutputDescInfo> &input_desc,
                                std::vector<InputOutputDescInfo> &output_desc) const;

  Status GetInputOutputDescInfo(std::vector<InputOutputDescInfo> &input_desc,
                                std::vector<InputOutputDescInfo> &output_desc,
                                std::vector<uint32_t> &input_formats, std::vector<uint32_t> &output_formats,
                                const bool by_dims) const;

  ///
  /// @ingroup ge
  /// @brief Get dynamic batch_info
  /// @param [out] batch_info
  /// @param [out] dynamic_type
  /// @return execute result
  ///
  Status GetDynamicBatchInfo(std::vector<std::vector<int64_t>> &batch_info, int32_t &dynamic_type) const;

  ///
  /// @ingroup ge
  /// @brief Get combined dynamic dims info
  /// @param [out] batch_info
  /// @return None
  ///
  void GetCombinedDynamicDims(std::vector<std::vector<int64_t>> &batch_info) const;

  void GetUserDesignateShapeOrder(std::vector<std::string> &user_input_shape_order) const;

  void GetCurrentShape(std::vector<int64_t> &batch_info, int32_t &dynamic_type) const;

  Status GetNodeAttr(const std::string &op_name, const std::string &attr_name, std::string &attr_info) const;

  void GetOutputShapeInfo(std::vector<std::string> &out_shape_info) const;

  ///
  /// @ingroup ge
  /// @brief Get AIPP input info
  /// @param [in] index
  /// @param [out] aipp_info
  /// @return execute result
  ///
  Status GetAippInfo(const uint32_t index, AippConfigInfo &aipp_info) const;

  Status GetAippType(const uint32_t index, InputAippType &aipp_type, size_t &aipp_index) const;

  ///
  /// @ingroup ge
  /// @brief Get model_id.
  /// @return model_id
  ///
  uint32_t GetModelId() const { return model_id_; }

  ///
  /// @ingroup ge
  /// @brief get unique identification for op when load two or more models
  /// @param [in] op_desc : current op.
  /// @param [in] std::string identification: unique identification for current op.
  /// @return None
  ///
  void GetUniqueId(const OpDescPtr &op_desc, std::string &unique_identification) const;

  void ReturnResult(const uint32_t data_id, const bool rslt_flg, const bool seq_end_flag, OutputData &output_data);

  void ReturnNoOutput(const uint32_t data_id);

  Status ModelRunStart();

  ///
  /// @ingroup ge
  /// @brief stop run model
  /// @return Status
  ///
  Status ModelRunStop();

  ///
  /// @ingroup ge
  /// @brief Get Session Id
  /// @return sessionID
  ///
  uint64_t GetSessionId() const { return session_id_; }

  const error_message::Context &GetErrorContext() const { return error_context_; }

  ///
  /// @ingroup ge
  /// @brief SetDeviceId
  /// @return void
  ///
  void SetDeviceId(const uint32_t device_id) { device_id_ = device_id; }

  void SetDieId(const int64_t die_id) { die_id_ = die_id; }

  ///
  /// @ingroup ge
  /// @brief Get device Id
  /// @return  device id
  ///
  uint32_t GetDeviceId() const {
    return (die_id_ == std::numeric_limits<int64_t>::max()) ? device_id_ : static_cast<uint32_t>(die_id_);
  }
  int64_t GetDieId() const { return die_id_; }

  bool NeedDestroyAicpuKernel() const { return need_destroy_aicpu_kernel_; }

  Status UpdateSessionId(const uint64_t session_id);

  const RuntimeParam &GetRuntimeParam() const { return runtime_param_; }

  int32_t GetDataInputTid() const { return dataInputTid; }
  void SetDataInputTid(const int32_t data_input_tid) { dataInputTid = data_input_tid; }

  void DisableZeroCopy(const void *const addr);

  void DisableZeroCopyInReuseMemoryMode(const NodePtr &node, const size_t idx, const void *const addr);

  bool GetOpDugReg() const { return is_op_debug_reg_; }

  Status ReportFusionOpInfo();

  Status ReportModelExtInfo();

  ///
  /// @ingroup ge
  /// @brief Save outside address of Data or NetOutput used info for ZeroCopy.
  /// @param [in] const OpDescPtr &op_desc: current op desc
  /// @param [in] const std::vector<void *> &outside_addrs: address of task
  /// @param [in] const void *args_offset: arguments address save the address.
  /// @return None.
  ///
  void SetZeroCopyAddr(const OpDescPtr &op_desc, const std::vector<void *> &outside_addrs, const void *const args_info,
                       const uintptr_t args_base, const size_t args_size, const size_t offset);

  void SetLogicalOutsideAddrs(const std::map<uintptr_t, std::set<size_t>> &args_offset,
                              const uintptr_t args_device_addr);

  std::set<size_t> GetZeroCopyArgsIndex(const std::vector<void *> &arg_logical_addrs) const;

  Status Mapping2BundleZeroCopy(const OpDescPtr op_desc,
                                const std::map<uintptr_t, std::set<size_t>> &args_offset, const size_t args_size,
                                const void *const args_host_copy, void *&args_device_addr, bool &own_memory);

  void SetDynamicSize(const std::vector<uint64_t> &batch_num, const int32_t dynamic_type);

  bool GetL1FusionEnableOption() const { return is_l1_fusion_enable_; }

  void SetProfileTime(const ModelProcStage stage, const uint64_t end_time = 0U);

  void SaveSpecifyAttrValues(const OpDescPtr &op_desc);

  Status ReportProfilingData();

  void SaveProfilingTask(const uint32_t op_idx, const domi::TaskDef &task_def, const TaskInfo &task_info);

  void SaveFftsPlusProfilingTask(const domi::TaskDef &task_def, const TaskInfo &task_info);

  void SaveDumpOpInfo(const OpDescPtr &op, const uint32_t task_id, const uint32_t stream_id) {
    exception_dumper_.SaveDumpOpInfo(runtime_param_, op, task_id, stream_id);
  }

  void SaveDumpTask(const uint32_t task_id, const uint32_t stream_id, const shared_ptr<OpDesc> &op_desc,
                    const uintptr_t args) {
    data_dumper_.SaveDumpTask(task_id, stream_id, op_desc, args);
  }

  Status DumpExceptionInfo(const std::vector<rtExceptionInfo> &exception_infos) const {
    return exception_dumper_.DumpExceptionInfo(exception_infos);
  }

  void DumperShrink() {
    data_dumper_.DumpShrink();
  }

  bool OpNeedDump(const std::string &op_name) {
    return GetDumpProperties().IsLayerNeedDump(dump_model_name_, om_name_, op_name);
  }

  bool ModelNeedDump();

  void SetEndGraphId(const uint32_t task_id, const uint32_t stream_id);
  DavinciModel &operator=(const DavinciModel &model) = delete;

  DavinciModel(const DavinciModel &model) = delete;

  const std::map<int64_t, std::vector<rtStream_t>> &GetHcclFolowStream() const {
    return main_follow_stream_mapping_;
  }
  void SaveHcclFollowStream(const int64_t main_stream_id, rtStream_t stream);

  void InitRuntimeParams();
  Status InitVariableMem();

  void UpdateMemBase(const uintptr_t mem_base) {
    runtime_param_.mem_base = mem_base;
    mem_base_ = mem_base;
  }
  void SetTotalArgsSize(const uint32_t args_size, const uint32_t mem_type = RT_MEMORY_HBM);
  uint32_t GetTotalArgsSize(const uint32_t mem_type = RT_MEMORY_HBM) const;
  void *GetCurrentArgsAddr(const uint32_t offset, const uint32_t mem_type = RT_MEMORY_HBM) const;
  void SetTotalIOAddrs(const std::vector<void *> &io_addrs, const uint32_t mem_type = RT_MEMORY_HBM);
  void SetHybridArgsSize(const uint32_t args_size) { hybrid_args_size_ += args_size; }
  uint32_t GetHybridArgsSize() const {
    return hybrid_args_size_;
  }
  void *GetCurrentHybridArgsAddr(const uint32_t offset) const {
    return ValueToPtr(hybrid_addrs_base_ + offset);
  }
  void SetTotalFixedAddrsSize(const std::string &tensor_name, const int64_t fix_addr_size);
  int64_t GetFixedAddrsSize(const std::string &tensor_name);
  void *GetCurrentFixedAddr(const int64_t offset) const {
    return ValueToPtr(fixed_addrs_base_ + static_cast<uint64_t>(offset));
  }

  uint32_t GetFixedAddrOutputIndex(const std::string &tensor_name) const {
    const auto it = tensor_name_to_peer_output_index_.find(tensor_name);
    if (it != tensor_name_to_peer_output_index_.end()) {
      return static_cast<uint32_t>(it->second);
    }
    return std::numeric_limits<uint32_t>::max();
  }
  void SetKnownNode(const bool known_node) { known_node_ = known_node; }
  bool IsKnownNode() const { return known_node_; }
  Status MallocKnownArgs(const domi::ModelTaskDef &model_task_def);
  Status UpdateKnownNodeArgs(const std::vector<void *> &inputs, const std::vector<void *> &outputs);
  Status UpdateRunAddr(std::vector<void *> &total_io_addrs, const bool update_args = true);
  Status UpdateZeroCopyAddr(std::vector<void *> &total_io_addrs, const std::vector<void *> &inputs,
                            const std::vector<void *> &outputs);
  Status GenInputOutputIndex();

  Status GetOrigInputInfo(const uint32_t index, OriginInputInfo &orig_input_info) const;
  Status GetAllAippInputOutputDims(const uint32_t index, std::vector<InputOutputDims> &input_dims,
                                   std::vector<InputOutputDims> &output_dims) const;

  // om file name
  void SetOmName(const std::string &om_name) { om_name_ = om_name; }
  void SetDumpModelName(const std::string &dump_model_name) { dump_model_name_ = dump_model_name; }

  void SetDumpProperties(const DumpProperties &dump_properties) { data_dumper_.SetDumpProperties(dump_properties); }
  const DumpProperties &GetDumpProperties() const { return data_dumper_.GetDumpProperties(); }

  bool GetOpDescInfo(const uint32_t stream_id, const uint32_t task_id, OpDescInfo &op_desc_info) const {
    return exception_dumper_.GetOpDescInfo(stream_id, task_id, op_desc_info);
  }
  void UpdateOpIOAddrs(const uint32_t task_id, const uint32_t stream_id, const std::vector<void *> &io_addrs);

  bool GetRunningFlag() const { return running_flg_; }
  void SetRunningFlag(const bool flag) { running_flg_ = flag; }
  Status SetRunAsyncListenerCallback(const RunAsyncCallback &callback);

  // for blocking aicpu op
  Status GetEventByStream(const rtStream_t stream, rtEvent_t &rt_event);
  Status GetEventIdForBlockingAicpuOp(const OpDescPtr &op_desc, const rtStream_t stream, uint32_t &event_id);

  uint32_t GetResultCode();
  Status ResetResult();
  Status GetAddrAndPrefCnt(const std::string &kernel_name, void *&addr, uint32_t &pref_cnt);

  void SetRootGraphId(const uint32_t root_graph_id) { runtime_param_.root_graph_id = root_graph_id; }

  Status ReportProfilingData(const uint32_t graph_id);

  bool HasZeroCopyAddr(const OpDescPtr &op_desc) const;

 private:
  // memory address of weights
  uintptr_t weights_mem_base_{0U};
  uintptr_t var_mem_base_{0U};
  // memory address of model
  uintptr_t fixed_mem_base_{0U};  // Initial of mem_base_, keep forever.
  uintptr_t mem_base_{0U};
  bool is_inner_mem_base_{false};
  bool is_inner_weight_base_{false};
  // input data manager
  DataInputer data_inputer_;
  uint64_t load_begin_time_{0U};
  uint64_t load_end_time_{0U};
  struct MsprofGeProfInferData prof_infer_data_{};
  int32_t dataInputTid{0};

  void *GetRunAddress(void *const addr) const;
  const void *GetLogicalAddress(void *const addr) const;

  ///
  /// @ingroup ge
  /// @brief Copy Check input size and model op size.
  /// @param [in] const int64_t &input_size: input size.
  /// @param [in] const int64_t &op_size: model op size.
  /// @param [in] is_dynamic: dynamic batch input flag.
  /// @return true if success
  ///
  bool CheckUserAndModelSize(const int64_t &size, const int64_t &op_size, const bool is_input,
                             const bool is_dynamic) const;

  ///
  /// @ingroup ge
  /// @brief Set copy only for No task feed NetOutput address.
  /// @return None.
  ///
  void SetCopyOnlyOutput();

  ///
  /// @ingroup ge
  /// @brief Copy Input/Output to model for direct use.
  /// @param [in] const InputData &input_data: user input data info.
  /// @param [in/out] OutputData &output_data: user output data info.
  /// @param [in] bool is_dynamic: whether is dynamic input, true: is dynamic input; false: not is dynamic input
  /// @return SUCCESS handle successfully / others handle failed
  ///
  Status CopyModelData(const InputData &input_data, OutputData &output_data, const bool is_dynamic);

  ///
  /// @ingroup ge
  /// @brief Copy Data addr to model for direct use.
  /// @param [in] data_info: model memory addr/size map { data_index, { tensor_size, tensor_addr } }.
  /// @param [in] is_input: input data or output data
  /// @param [in] blobs: user input/output data list.
  /// @param [in] is_dynamic: whether is dynamic input, true: is dynamic input; false: not is dynamic input
  /// @param [in] batch_label: batch label for multi-batch scenes
  /// @return SUCCESS handle successfully / others handle failed
  ///
  Status UpdateIoTaskArgs(const std::map<uint32_t, ZeroCopyOffset> &data_info, const bool is_input,
                          const std::vector<DataBuffer> &blobs, const bool is_dynamic, const std::string &batch_label);

  Status HandleInputData(InputData &input_data);

  Status CopyInputData(const InputData &input_data);

  Status CopyOutputData(const OutputData &output_data, const rtMemcpyKind_t kind);

  Status SyncVarData();

  Status InitWeightMem(const uintptr_t mem_ptr, const uintptr_t weight_ptr, const size_t weight_size);
  Status InitFeatureMapAndP2PMem(const uintptr_t mem_ptr, const size_t mem_size);

  void CreateInputDimsInfo(const OpDescPtr &op_desc, const Format format, ShapeDescription &shape_info,
                           ShapeDescription &dims_info) const;

  void SetInputDimsInfo(const std::vector<int64_t> &input_dims, const Format format,
                        ShapeDescription &shape_info) const;

  Status GetInputDescInfo(std::vector<InputOutputDescInfo> &input_desc,
                          std::vector<uint32_t> &input_format, const bool by_dims) const;
  Status GetOutputDescInfo(std::vector<InputOutputDescInfo> &output_desc, std::vector<uint32_t> &output_format) const;

  Status InitTaskInfo(const domi::ModelTaskDef &model_task_def);

  void UnbindHcomStream();

  Status DistributeTask(const domi::ModelTaskDef &model_task_def);

  void SaveProfilingTaskDescInfo(const OpDescPtr &op_desc, const TaskInfo &task_info, const domi::TaskDef &task_def);

  uint8_t *MallocFeatureMapMem(const size_t data_size) const;

  uint8_t *MallocWeightsMem(const size_t weights_size) const;

  Status MallocExMem();

  void FreeFeatureMapMem();

  void FreeWeightsMem();

  void FreeExMem();

  void ReleaseTask();

  void ClearTaskAddrs();

  void UnbindTaskSinkStream();

  bool IsAicpuKernelConnectSpecifiedLayer() const;

  ///
  /// @ingroup ge
  /// @brief Reduce memory usage after task sink.
  /// @return: void
  ///
  void Shrink();

  Status InitIoNodes(const ComputeGraphPtr &compute_graph, std::vector<NodePtr> &variable_nodes);

  ///
  /// @ingroup ge
  /// @brief Travel all nodes and do some init.
  /// @param [in] compute_graph: ComputeGraph to load.
  /// @return Status
  ///
  Status InitNodes(const ComputeGraphPtr &compute_graph);

  ///
  /// @ingroup ge
  /// @brief Data Op Initialize.
  /// @param [in] ComputeGraphPtr: root graph of the model.
  /// @param [in] NodePtr: Data Op.
  /// @param [in/out] data_op_index: index of courrent count.
  /// @param [in/out] data_by_index: Data ordered by index.
  /// @return Status
  ///
  Status InitDataOp(const ComputeGraphPtr &graph, const NodePtr &node, uint32_t &data_op_index,
                    std::map<uint32_t, OpDescPtr> &data_by_index, std::set<const void *> &input_outside_addrs);

  ///
  /// @ingroup ge
  /// @brief Sort Data op list by index.
  /// @param [in] data_by_index: map of Data Op.
  /// @param [in] output_op_list: list of NetOutput op.
  /// @return Status
  ///
  Status GenInputOutputInfo(const std::map<uint32_t, OpDescPtr> &data_by_index,
                            const std::vector<OpDescPtr> &output_op_list);

  ///
  /// @ingroup ge
  /// @brief NetOutput Op Initialize.
  /// @param [in] ComputeGraphPtr: root graph of the model.
  /// @param [in] NodePtr: NetOutput Op.
  /// @param [in/out] std::vector<OpDescPtr>: All NetOutput node in model.
  /// @return Status
  ///
  Status InitNetOutput(const ComputeGraphPtr &graph, const NodePtr &node, std::vector<OpDescPtr> &output_op_list,
                       std::set<const void *> &output_outside_addrs);

  ///
  /// @ingroup ge
  /// @brief Constant Op Init.
  /// @return Status
  ///
  Status InitConstant(const OpDescPtr &op_desc);

  Status InitVariable(const OpDescPtr &op_desc, std::map<std::string, OpDescPtr> &variable_by_name);

  /// @ingroup ge
  /// @brief Get Op rtStream.
  /// @param [in] op_desc: Op descriptor.
  /// @param [in] stream_id: Logical stream id.
  /// @param [out] stream: rt stream.
  /// @return Status
  Status GetOpStream(const OpDescPtr &op_desc, const size_t stream_id, rtStream_t &stream);

  /// @ingroup ge
  /// @brief LabelSet Op Initialize.
  /// @param [in] op_desc: LabelSet Op descriptor.
  /// @return Status
  Status InitLabelSet(const OpDescPtr &op_desc);

  Status InitStreamSwitch(const OpDescPtr &op_desc);

  Status InitStreamActive(const OpDescPtr &op_desc);

  ///
  /// @ingroup ge
  /// @brief Case Op Init.
  /// @return Status
  ///
  Status InitCase(const OpDescPtr &op_desc);

  Status SetDynamicBatchInfo(const OpDescPtr &op_desc, const uint32_t batch_num);

  ///
  /// @ingroup ge
  /// @brief TVM Op Init.
  /// @return Status
  ///
  Status InitTbeHandle(const OpDescPtr &op_desc);

  ///
  /// @ingroup ge
  /// @brief Make active stream list and bind to model.
  /// @return: 0 for success / others for fail
  ///
  Status BindModelStream();

  ///
  /// @ingroup ge
  /// @brief Init model stream for NN model.
  /// @return Status
  ///
  Status InitModelStream(const rtStream_t stream);

  ///
  /// @ingroup ge
  /// @brief ACL, Load task list with queue entrance.
  /// @return: 0 for success / others for fail
  ///
  Status LoadWithQueue();

  ///
  /// @ingroup ge
  /// @brief ACL, Bind Data Op addr to input queue.
  /// @return: 0 for success / others for fail
  ///
  Status BindInputQueue();

  Status BindControlInputQueue();

  Status CpuTaskModelZeroCopy(std::vector<uintptr_t> &mbuf_list,
                              const std::map<uint32_t, ZeroCopyOffset> &outside_addrs,
                              const std::vector<bool> &is_no_tiling_list,
                              ZeroCpyArgs &cpy_args);

  ///
  /// @ingroup ge
  /// @brief ACL, Bind NetOutput Op addr to output queue.
  /// @return: 0 for success / others for fail
  ///
  Status BindOutputQueue();
  Status CpuModelPrepareOutput(const size_t output_idx, const uintptr_t addr, const uint32_t data_size);

  ///
  /// @ingroup ge
  /// @brief definiteness queue schedule, bind input queue to task.
  /// @param [in] queue_id: input queue id from user.
  /// @param [in] addr: Data Op output tensor address.
  /// @param [in] size: Data Op output tensor size.
  /// @return: 0 for success / others for fail
  ///
  Status CpuModelDequeue(const uint32_t queue_id);

  ///
  /// @ingroup ge
  /// @brief definiteness queue schedule, active original model stream.
  /// @return: 0 for success / others for fail
  ///
  Status CpuActiveStream();

  ///
  /// @ingroup ge
  /// @brief definiteness queue schedule, wait for end graph.
  /// @return: 0 for success / others for fail
  ///
  Status CpuWaitEndGraph();

  ///
  /// @ingroup ge
  /// @brief definiteness queue schedule, post process after end graph.
  /// @return: 0 for success / others for fail
  ///
  Status CpuPostProcess();
  Status CpuModelPostProcess(const size_t ouput_idx, const uintptr_t addr,
                             const uint32_t data_size, const ProcessStage stage);

  Status BindEnqueue();
  Status CpuModelEnqueue(const uint32_t queue_id, const uintptr_t out_mbuf);

  ///
  /// @ingroup ge
  /// @brief definiteness queue schedule, repeat run model.
  /// @return: 0 for success / others for fail
  ///
  Status CpuModelRepeat();

  Status InitEntryTask();
  Status AddHeadStream();

  ///
  /// @ingroup ge
  /// @brief set ts device.
  /// @return: 0 for success / others for fail
  ///
  Status SetTSDevice();

  Status OpDebugRegister();

  void OpDebugUnRegister();

  Status DoTaskSink();

  void CreateOutput(const size_t index, const OpDescPtr &op_desc, InputOutputDescInfo &output,
                    uint32_t &format_result) const;

  Status TransAllVarData(const ComputeGraphPtr &graph, const std::vector<NodePtr> &variable_nodes) const;

  void SetDataDumperArgs(const ComputeGraphPtr &graph, const std::map<std::string, OpDescPtr> &variable_by_name);

  Status InitL1DataDumperArgs();

  Status InitFusionProfiling(const FusionOpInfo &fusion_op_info);

  void SinkTimeProfile(const uint32_t data_index, const uint64_t request_id);

  void DisableZeroCopyNode(const OpDescPtr &op_desc);

  Status InitOutputTensorInfo(const OpDescPtr &op_desc);
  Status GenOutputTensorInfo(OutputData &output_data, std::vector<Tensor> &outputs);
  Status BuildOutputShapeInfo(const size_t output_idx, std::vector<int64_t> &output_shape, int64_t &output_size);

  Status InitInputDescInfo(const OpDescPtr &op_desc);
  Status InitOutputDescInfo(const OpDescPtr &op_desc, const std::vector<std::string> &out_node_name);

  Status InitOrigInputInfo(const uint32_t index, const OpDescPtr &op_desc);
  Status InitAippInfo(const uint32_t index, const OpDescPtr &op_desc);
  Status InitAippType(const uint32_t index, const OpDescPtr &op_desc, const std::map<uint32_t, OpDescPtr> &data_list);
  Status InitAippInputOutputDims(const uint32_t index, const OpDescPtr &op_desc);

  void ParseAIPPInfo(const std::string in_out_info, InputOutputDims &dims_info) const;
  void SetLabelForDynamic(const NodePtr &node);

  ///
  /// @ingroup domi_ome
  /// @brief Get cur_dynamic_dims for all input.
  /// @param [in] std::vector<vector<int64_t>> &tensor_input_dims: dims info of all user_inputs.
  /// @param [out] std::vector<int32_t> &cur_dynamic_dims: real dims gather, where the index of -1.
  /// @return 0: SUCCESS / others: INTERNAL_ERROR
  ///
  Status GetCurDynamicDims(const std::vector<std::vector<int64_t>> &tensor_input_dims,
                           std::vector<int32_t> &cur_dynamic_dims) const;

  void ParseInputsDimsForData(const std::vector<std::vector<int64_t>> &tensor_input_dims,
                              std::vector<std::vector<int64_t>> &real_input_dims) const;
  Status ParseInputsDimsForGetNextNoSinkAndData(const std::vector<NodePtr> &dynamic_nodes,
                                                const std::vector<std::vector<int64_t>> &tensor_input_dims,
                                                std::vector<std::vector<int64_t>> &real_input_dims) const;
  Status ParseInputsDims(const std::vector<std::vector<int64_t>> &tensor_input_dims,
                         std::vector<std::vector<int64_t>> &real_input_dims) const;

  void ParseDynamicOutShape(const std::vector<std::string> &str_info,
                            std::vector<std::vector<int64_t>> &vec_info) const;
  bool IsGetNextSinkDynamic(const OpDescPtr &op_desc) const;

  Status InitRealSizeAndShapeInfo(const ComputeGraphPtr &compute_graph, const NodePtr &node);
  void GetAllGearsInfo(const NodePtr &node);
  Status GetDynamicDimsNodeInfo(const NodePtr &node);
  Status GetGearAndRealOutSizeInfo(const ComputeGraphPtr &graph, const NodePtr &node);
  Status GetRealOutputSizeOfCase(const ComputeGraphPtr &graph, const size_t input_index, const NodePtr &case_node);
  Status GetGearAndRealOutShapeInfo(const NodePtr &node);
  void BuildZeroCopyTasksLookupTable();
  void UpdateZeroCopyTaskParam(const std::pair<uint32_t, ZeroCopyOffset> &data,
                               const DataBuffer &buffer, const std::string &batch_label, const bool is_input);

  Status AllocateResource(const Node &node);
  Status AllocateQueueResource(const Node &node, const OpDescPtr &op_desc, const NamedAttrs &resource);
  Status AllocateDvppChlResource(const OpDescPtr &op_desc);
  Status AllocateVdecChlResource(const OpDescPtr &op_desc);

  Status UpdateOpInputValue(const OpDescPtr &op_desc, const int32_t input_index, const uint32_t queue_id) const;

  Status InitFileConstant(const NodePtr &node);

  Status InitQueueDataNodes(const std::vector<NodePtr> &queue_data_nodes,
                            const uint32_t data_index,
                            std::set<const void *> &input_outside_addrs);

  Status InitInputZeroCopy(const OpDescPtr &op_desc, const uint32_t data_index,
                           std::set<const void *> &input_outside_addrs);

  void ProfFusionOpInfo(const OpDescPtr &op_desc, const FusionOpInfo &fusion_op_info, ProfileInfo &profile) const;

  static size_t GetMemoryTypeIndex(const uint32_t memory_type);

  bool is_weight_mem_has_inited_{false};
  bool is_feature_map_mem_has_inited_{false};

  uint32_t model_id_{0U};
  uint32_t runtime_model_id_{0U};
  uint32_t sub_model_id_{0U};
  std::string name_;

  // used for inference data dump
  std::string om_name_;
  std::string dump_model_name_;

  uint32_t version_{0U};
  GeModelPtr ge_model_;  // release after DavinciModel::Init

  bool need_destroy_aicpu_kernel_{false};

  std::map<int64_t, OpDescPtr> op_list_;  // release after DavinciModel::Init

  uintptr_t global_step_addr_{0U};
  uint64_t global_step_size_{0U};

  std::map<uint32_t, ZeroCopyOffset> input_data_info_;
  std::map<uint32_t, ZeroCopyOffset> output_data_info_;

  std::set<const void *> real_virtual_addrs_;

  // output op: save cce op actual needed memory size
  std::vector<int64_t> output_memory_size_list_;

  std::thread thread_id_;

  std::shared_ptr<ModelListener> listener_;

  bool run_flg_{false};
  // check whether model is running with data
  bool running_flg_{false};

  std::mutex mux_run_flg_;

  int32_t priority_{0};

  std::vector<rtStream_t> stream_list_;

  std::mutex all_hccl_stream_list_mutex_;
  std::vector<rtStream_t> all_hccl_stream_list_;

  // for reuse hccl_follow_stream
  std::mutex capacity_of_stream_mutex_;
  std::map<int64_t, std::vector<rtStream_t>> main_follow_stream_mapping_;

  std::vector<rtEvent_t> event_list_;

  std::vector<rtLabel_t> label_list_;
  std::set<uint32_t> label_id_indication_;

  std::mutex label_args_mutex_;
  std::map<uint32_t, std::pair<void *, uint32_t>> label_goto_args_;

  std::mutex outside_addrs_mutex_;
  std::vector<ZeroCopyTask> zero_copy_tasks_;  // Task used Data or NetOutput addr.

  std::mutex bundle_zero_copy_tasks_mutex_;
  std::vector<ZeroCopyTask> bundle_zero_copy_tasks_;

  std::set<const void *> copy_only_addrs_;     // Address need copy to original place.

  std::vector<TaskInfoPtr> task_list_;
  // rt_moodel_handle
  rtModel_t rt_model_handle_{nullptr};

  rtStream_t rt_model_stream_{nullptr};

  bool is_inner_model_stream_{false};

  bool is_async_mode_{false};  // For NN execute, Async mode use rtMemcpyAsync on rt_model_stream_.
  ExecuteMode last_execute_mode_{ExecuteMode::INITIALIZATION};

  bool is_stream_list_bind_{false};
  bool is_pure_head_stream_{false};
  rtStream_t rt_head_stream_{nullptr};
  rtStream_t rt_entry_stream_{nullptr};
  rtAicpuDeployType_t deploy_type_{AICPU_DEPLOY_RESERVED};

  // ACL queue schedule, save queue ids for Init.
  std::vector<TaskInfoPtr> cpu_task_list_;
  std::vector<uint32_t> input_queue_ids_;    // input queue ids created by caller.
  std::vector<uint32_t> output_queue_ids_;   // output queue ids created by caller.
  std::vector<uintptr_t> input_mbuf_list_;   // input mbuf created by dequeue task.
  std::vector<uintptr_t> output_mbuf_list_;  // output mbuf created by dequeue task.

  uint64_t session_id_{0U};
  error_message::Context error_context_;

  uint32_t device_id_{0U};
  int64_t die_id_ = std::numeric_limits<int64_t>::max();

  std::mutex flowctrl_op_index_internal_map_mutex_;
  std::map<uint32_t, uint32_t> flowctrl_op_index_internal_map_;

  std::vector<rtStream_t> active_stream_list_;
  std::set<uint32_t> active_stream_indication_;

  std::set<uint32_t> hcom_streams_;
  RuntimeParam runtime_param_;

  TBEKernelHandle bin_kernel_handle_;

  // for profiling task and graph info
  std::vector<TaskDescInfo> task_desc_info_;

  // for data dump
  DataDumper data_dumper_;
  ExceptionDumper exception_dumper_;
  OpdebugRegister opdebug_register_;
  uint64_t iterator_count_{0U};
  bool is_l1_fusion_enable_{false};
  std::map<OpDescPtr, void *> saved_task_addrs_;  // release after DavinciModel::Init
  void *l1_fusion_addr_ = nullptr;

  bool known_node_ = false;
  std::vector<uintptr_t> known_args_bases_{0U, 0U};
  std::vector<uint32_t> known_args_sizes_{0U, 0U};
  uintptr_t hybrid_addrs_base_{0U};
  uint32_t hybrid_args_size_{0U};
  uintptr_t fixed_addrs_base_{0U};
  int64_t fixed_addr_size_{0};
  std::unordered_map<const void *, uint32_t> known_input_idx_;
  std::unordered_map<const void *, uint32_t> known_output_idx_;
  std::vector<void *> user_input_addrs_;
  std::vector<void *> user_output_addrs_;
  std::vector<std::vector<void *>> total_io_addrs_{{}, {}};

  OmeContext run_context_;
  std::vector<std::vector<int64_t>> batch_info_;
  std::vector<std::vector<int64_t>> combined_batch_info_;
  std::vector<std::string> user_designate_shape_order_;
  int32_t dynamic_type_ = 0;
  bool is_dynamic_ = false;

  std::vector<uint64_t> batch_size_;
  // key: input tensor name, generally rts op;
  // value: the fixed addr of input anchor, same as the peer output anchor addr of the peer op
  std::map<std::string, int64_t> tensor_name_to_fixed_addr_size_;

  // key: input tensor name, generally rts op; value: the peer output anchor of the peer op
  std::map<std::string, int64_t> tensor_name_to_peer_output_index_;
  // if model is first execute
  bool is_first_execute_{true};
  // for op debug
  bool is_op_debug_reg_ = false;
  bool is_online_infer_dynamic_ = false;
  bool is_getnext_sink_dynamic_ = false;
  std::vector<int32_t> cur_dynamic_dims_;
  void *netoutput_last_input_addr_ = nullptr;
  int64_t netoutput_last_input_size_ = 0;
  size_t shape_of_cur_dynamic_dims_ = 0U;
  // key: input_index: input is merge node; value: each gear info and each output size
  std::map<size_t, std::map<std::vector<int32_t>, int64_t>> merge_nodes_gear_and_real_out_size_info_;
  // key: input_index: input is merge node; value: each gear info and each output shape
  std::map<size_t, std::map<std::vector<int32_t>, std::vector<int64_t>>> merge_nodes_gear_and_real_out_shape_info_;
  std::vector<std::vector<int32_t>> all_gears_info_;

  std::vector<ProfileInfo> profile_list_;

  // For super kernel.
  SuperKernelTaskInfo skt_info_;

  bool has_output_node_ = false;
  bool is_dynamic_aipp_ = false;
  std::vector<std::string> dynamic_output_shape_info_;

  std::vector<std::vector<void *>> input_addrs_list_;
  std::vector<std::vector<void *>> output_addrs_list_;

  std::vector<int64_t> output_buffer_size_;
  std::vector<GeShape> output_shape_info_;

  std::vector<bool> input_no_tiling_flag_;
  bool has_no_tiling_input_ = false;
  std::vector<bool> output_no_tiling_flag_;
  bool has_no_tiling_output_ = false;
  std::map<uint32_t, void *> output_no_tiling_data_addr_;

  std::map<uint32_t, OriginInputInfo> orig_input_info_;
  std::map<uint32_t, AippConfigInfo> aipp_info_list_;
  std::map<uint32_t, std::pair<InputAippType, size_t>> aipp_type_list_;
  std::map<uint32_t, std::pair<std::vector<InputOutputDims>, std::vector<InputOutputDims>>> aipp_dims_info_;

  std::vector<InputOutputDescInfo> input_descs_;
  std::vector<InputOutputDescInfo> input_descs_dims_;
  std::vector<uint32_t> input_formats_;
  std::vector<InputOutputDescInfo> output_descs_;
  std::vector<uint32_t> output_formats_;

  // op name to attrs mapping
  std::map<std::string, std::map<std::string, std::vector<std::string>>> op_name_to_attrs_;

  std::map<rtStream_t, rtEvent_t> stream_2_event_;

  // fields for build fast search hash table for zero copy tasks
  std::mutex lookup_table_build_lock_;
  bool lookup_table_built_{false};
  std::unordered_map<std::string, std::unordered_set<ZeroCopyTask*>> label2tasks_;
  std::unordered_map<uintptr_t, std::unordered_set<ZeroCopyTask*>> addr2specific_label_tasks_;
  std::unordered_map<uintptr_t, std::unordered_set<ZeroCopyTask*>> addr2default_label_tasks_;
  AiCpuResources aicpu_resources_;
  std::map<std::string, std::string> file_id_and_path_map_;

  // for support overflow detection
  void *globalworkspace_overflow_addr_ = nullptr;
  bool use_control_input_queue_ = false;
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_DAVINCI_MODEL_H_
