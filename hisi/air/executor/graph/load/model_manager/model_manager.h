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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_MODEL_MANAGER_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_MODEL_MANAGER_H_

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <thread>

#include "cce/fwk_adpt_struct.h"
#include "common/properties_manager.h"
#include "common/model/ge_root_model.h"
#include "external/ge/ge_api_types.h"
#include "framework/common/ge_inner_error_codes.h"
#include "framework/common/ge_types.h"
#include "framework/common/helper/model_helper.h"
#include "framework/common/helper/om_file_helper.h"
#include "framework/common/types.h"
#include "graph/model.h"
#include "exec_runtime/deploy/model_deployer.h"
#include "graph/manager/graph_manager_utils.h"
#include "graph/load/model_manager/davinci_model.h"
#include "graph/load/model_manager/heterogeneous_model_executor.h"
#include "hybrid/hybrid_davinci_model.h"
#include "mmpa/mmpa_api.h"

namespace ge {
class ModelManager {
 public:
  static ModelManager &GetInstance();

  ///
  /// @ingroup domi_ome
  /// @brief load and init model without input or output queue.
  /// @param [out] model_id: model id.
  /// @param [in] root_model: RootModel load from offline model file.
  /// @param [in] priority: model priority.
  /// @return Status run result
  /// @author @
  ///
  Status LoadModelWithoutQ(uint32_t &model_id, const GeRootModelPtr &root_model, const int32_t priority = 0);

  ///
  /// @ingroup domi_ome
  /// @brief load and init model
  /// @param [in] model_id model id
  /// @param [in] model including model ptr and size
  /// @param [in] listener used to return result
  /// @param [in/out] info model task generate info
  /// @return Status run result
  /// @author
  ///
  Status LoadModelOffline(uint32_t &model_id, const ModelData &model,
                          std::shared_ptr<ModelListener> listener = nullptr,
                          const uintptr_t mem_ptr = 0U, const size_t mem_size = 0U,
                          const uintptr_t weight_ptr = 0U, const size_t weight_size = 0U);

  ///
  /// @ingroup domi_ome
  /// @brief load and init model and this function may be used in multi-threaded scenarios, so pay attention
  /// @param [out] model_id model id
  /// @param [in] model modeldef datatype
  /// @param [in] listener used to return result
  /// @param [in] isTrainMode model type
  /// @return Status run result
  /// @author @
  ///
  Status LoadModelOnline(uint32_t &model_id, const GeRootModelPtr &ge_root_model, const GraphNodePtr &graph_node,
                         const uint32_t device_id, const int64_t die_id);

  Status LoadFlowModelOnline(uint32_t &model_id, const FlowModelPtr &ge_root_model, const GraphNodePtr &graph_node);

  Status DoLoadHybridModelOnline(const uint32_t model_id, const std::string &om_name,
                                 const GeRootModelPtr &ge_root_model,
                                 const std::shared_ptr<ModelListener> &listener);

  ///
  /// @ingroup ge
  /// @brief ACL case, Load task list with queue.
  /// @param [out] model_id: model id for manager.
  /// @param [in] model_data: Model data load from offline model file.
  /// @param [in] input_que_ids: input queue ids from user, num equals Data Op.
  /// @param [in] output_que_ids: input queue ids from user, num equals NetOutput Op.
  /// @return: 0 for success / others for fail
  ///
  Status LoadModelWithQ(uint32_t &model_id, const ModelData &model_data,
                        const std::vector<uint32_t> &input_queue_ids,
                        const std::vector<uint32_t> &output_queue_ids);

  ///
  /// @ingroup ge
  /// @brief ACL case, Load task list with queue.
  /// @param [out] model_id: model id for manager.
  /// @param [in] root_model: RootModel load from offline model file.
  /// @param [in] input_que_ids: input queue ids from user, num equals Data Op.
  /// @param [in] output_que_ids: input queue ids from user, num equals NetOutput Op.
  /// @param [in] priority: model priority.
  /// @return: 0 for success / others for fail
  ///
  Status LoadModelWithQ(uint32_t &model_id,
                        const GeRootModelPtr &root_model,
                        const std::vector<uint32_t> &input_queue_ids,
                        const std::vector<uint32_t> &output_queue_ids,
                        const int32_t priority = 0,
                        const bool need_update_session_id = true);

  ///
  /// @ingroup domi_ome
  /// @brief unload model and free resources
  /// @param [in] model_id model id
  /// @return Status run result
  /// @author
  ///
  Status Unload(const uint32_t model_id);

  ///
  /// @ingroup domi_ome
  /// @brief process input data and run asynchronously
  /// cannot be invoked by multiple thread
  /// if one fails, other continue
  /// @param [in] input_data   input data
  /// @return SUCCESS          success
  /// @return PARAM_INVALID    parameter invalid
  /// @return MODEL_NOT_READY  model not ready
  /// @return PUSH_DATA_FAILED push data into model queue failed
  /// @author
  ///
  Status SyncExecuteModel(const InputData &input_data, OutputData &output_data);

  Status DataInputTensor(const uint32_t model_id, const std::vector<Tensor> &inputs);

  ///
  /// @ingroup domi_ome
  /// @brief model start to run
  ///
  Status Start(const uint32_t model_id);

  ///
  /// @ingroup domi_ome
  /// @brief  ACL case, do not start new thread, return result
  /// @param [in] model_id  model id
  /// @param [in] stream   model stream
  /// @param [in] async_mode  is asynchronize mode.
  /// @param [in] input_data  model input data
  /// @param [in] input_desc  description of model input data
  /// @param [out] output_data  model output data
  /// @param [out] output_desc  description of model output data
  ///
  Status ExecuteModel(const uint32_t model_id, const rtStream_t stream, const bool async_mode,
                      const InputData &input_data, const std::vector<GeTensorDesc> &input_desc,
                      OutputData &output_data, std::vector<GeTensorDesc> &output_desc);

  ///
  /// @ingroup domi_ome
  /// @brief  ACL case, do not start new thread, return result
  /// @param [in] model_id  model id
  /// @param [in] stream   model stream
  /// @param [in] async_mode  is asynchronize mode.
  /// @param [in] inputs  model inputs
  /// @param [in] outputs  model outputs
  ///
  Status ExecuteModel(const uint32_t model_id, const rtStream_t stream, const bool async_mode,
                      const std::vector<GeTensor> &input_tensor, const std::vector<GeTensor> &output_tensor);

  Status SyncExecuteModel(const uint32_t model_id, const std::vector<GeTensor> &inputs, std::vector<GeTensor> &outputs);

  ///
  /// @ingroup domi_ome
  /// @brief model stop
  ///
  Status Stop(const uint32_t model_id);

  ///
  /// @ingroup domi_ome
  /// @brief comment handle function
  ///
  static Status HandleCommand(const Command &cmd_info);

  ///
  /// @ingroup domi_ome
  /// @brief get model memory usage
  /// @param [in] model_id  model id
  /// @return SUCCESS          success
  /// @return PARAM_INVALID    parameter invalid
  ///
  Status GetMaxUsedMemory(const uint32_t model_id, uint64_t &max_size);

  ///
  /// @ingroup domi_ome
  /// @brief get model input and output size
  /// @param [in] model_id  model id
  /// @param [out] input_shape   input tensor
  /// @param [out] output_shape  output tensor
  /// @return SUCCESS          success
  /// @return PARAM_INVALID    parameter invalid
  ///
  Status GetInputOutputDescInfo(const uint32_t model_id, std::vector<InputOutputDescInfo> &input_desc,
                                std::vector<InputOutputDescInfo> &output_desc);

  Status GetInputOutputDescInfo(const uint32_t model_id, std::vector<InputOutputDescInfo> &input_desc,
                                std::vector<InputOutputDescInfo> &output_desc, std::vector<uint32_t> &inputFormats,
                                std::vector<uint32_t> &outputFormats, const bool new_model_desc = false);

  ///
  /// @ingroup ge
  /// @brief Get dynamic batch_info
  /// @param [in] model_id
  /// @param [out] batch_info
  /// @param [out] dynamic_type
  /// @return execute result
  ///
  Status GetDynamicBatchInfo(const uint32_t model_id, std::vector<std::vector<int64_t>> &batch_info,
                             int32_t &dynamic_type);

  ///
  /// @ingroup ge
  /// @brief Get combined dynamic dims info
  /// @param [in] model_id
  /// @param [out] batch_info
  /// @return execute result
  ///
  Status GetCombinedDynamicDims(const uint32_t model_id, std::vector<std::vector<int64_t>> &batch_info);

  ///
  /// @ingroup ge
  /// @brief Get user designate shape order
  /// @param [in] model_id
  /// @param [out] user_input_shape_order
  /// @return execute result
  ///
  Status GetUserDesignateShapeOrder(const uint32_t model_id, std::vector<std::string> &user_input_shape_order);

  ///
  /// @ingroup ge
  /// @brief Get AIPP info
  /// @param [in] model_id
  /// @param [in] index
  /// @param [out] aipp_info
  /// @return execute result
  ///
  Status GetAippInfo(const uint32_t model_id, const uint32_t index, AippConfigInfo &aipp_info);

  Status GetAippType(const uint32_t model_id, const uint32_t index, InputAippType &type, size_t &aipp_index);

  Status GetCurrentShape(const uint32_t model_id, std::vector<int64_t> &batch_info, int32_t &dynamic_type);

  Status GetNodeAttr(const uint32_t model_id, const std::string &op_name, const std::string &attr_name,
                     std::string &attr_info);

  Status GetOutputShapeInfo(const uint32_t model_id, std::vector<std::string> &dynamic_output_shape_info);

  Status SetDynamicSize(const uint32_t model_id, const std::vector<uint64_t> &batch_num, const int32_t dynamic_type);

  ///
  /// @ingroup domi_ome
  /// @brief Get model according to given id
  ///
  std::shared_ptr<DavinciModel> GetModel(const uint32_t id);

  std::shared_ptr<hybrid::HybridDavinciModel> GetHybridModel(const uint32_t id);

  Status KernelLaunchEx(const aicpu::FWKAdapter::FWKOperateType op_type, const uint64_t session_id,
                        const uint32_t model_id, const uint32_t sub_model_id);

  Status CreateAicpuSession(const uint64_t session_id);

  static Status GetModelMemAndWeightSize(const ModelData &model, size_t &mem_size, size_t &weight_size);

  void DestroyAicpuSession(const uint64_t session_id);

  Status DestroyAicpuKernel(const uint64_t session_id, const uint32_t model_id, const uint32_t sub_model_id);

  void CreateAicpuKernel(const uint64_t session_id, const uint32_t model_id, const uint32_t sub_model_id,
                         const uint64_t kernel_id);

  Status DestroyAicpuSessionForInfer(const uint32_t model_id);

  Status LoadCustAicpuSo(const OpDescPtr &op_desc, const std::string &so_name, bool &loaded);
  Status LoadCustAicpuSo(const CustAICPUKernelPtr &aicpu_kernel, const std::string &so_name, bool &loaded);

  Status LaunchCustAicpuSo();

  Status ClearAicpuSo();

  Status LaunchKernelCustAicpuSo(const std::string &kernel_name);

  Status LaunchKernelCheckAicpuOp(const std::vector<std::string> &aicpu_optype_list,
                                  const std::vector<std::string> &aicpu_tf_optype_list);

  Status CheckAicpuOpList(const GeModelPtr ge_model);

  Status GetOrigInputInfo(const uint32_t model_id, const uint32_t index, OriginInputInfo &orig_input_info);

  void GenSessionId(uint64_t &session_id);

  Status GetAllAippInputOutputDims(const uint32_t model_id, const uint32_t index,
                                   std::vector<InputOutputDims> &input_dims,
                                   std::vector<InputOutputDims> &output_dims);

  bool IsDynamicShape(const uint32_t model_id);
  bool IsNeedHybridLoad(const GeRootModel &ge_root_model) const;
  Status GetOpDescInfo(const uint32_t device_id, const uint32_t stream_id, const uint32_t task_id,
                       OpDescInfo &desc_info) const;

  Status EnableExceptionDump(const std::map<std::string, std::string> &run_options);

  const std::vector<rtExceptionInfo> &GetExceptionInfos() const { return exception_infos_; }

  void AddExceptionInfo(const rtExceptionInfo &exception_data) {
    const std::lock_guard<std::mutex> lk(exception_infos_mutex_);
    exception_infos_.emplace_back(exception_data);
  }

  static void ExceptionCallback(rtExceptionInfo *exception_data) {
    ModelManager::GetInstance().AddExceptionInfo(*exception_data);
  }

  bool IsDumpExceptionOpen() const { return dump_exception_flag_; }

  bool IsSocketClose() const { return is_socket_close_; }

  void SetSocketCloseStatus(const bool status) { is_socket_close_ = status; }

  bool IsHeterogeneous(const uint32_t model_id) const;

  std::shared_ptr<HeterogeneousModelExecutor> GetHeterogeneousModelExecutor(const uint32_t model_id);

  Status ExecuteHeterogeneousModel(const uint32_t model_id,
                                   const std::vector<GeTensor> &inputs,
                                   std::vector<GeTensor> &outputs);

  uint32_t GetRunningFlag(const uint32_t model_id);
  uint32_t GetDataInputerSize(const uint32_t model_id);

  Status SetCallback(const uint32_t model_id, const GeRootModelPtr &ge_root_model, const RunAsyncCallback &callback);

  Status ModelSubscribe(const uint32_t graph_id);

 private:
  ///
  /// @ingroup domi_ome
  /// @brief constructor
  ///
  ModelManager();

  ///
  /// @ingroup domi_ome
  /// @brief destructor
  ///
  ~ModelManager();

  ///
  /// @ingroup domi_ome
  /// @brief insert new model into model manager set
  ///
  void InsertModel(const uint32_t model_id, const std::shared_ptr<DavinciModel> &davinci_model);
  void InsertModel(const uint32_t model_id, const std::shared_ptr<hybrid::HybridDavinciModel> &hybrid_model);

  Status CheckAndReleaseStreamEventResource(const GeModelPtr &ge_model, const uint32_t model_id);
  Status ReleaseResource(const uint64_t need_resource, uint64_t free_resource, const std::string &resource_kind);
  Status GetFreeStream(uint64_t &free_stream);
  void GetFreeEvent(uint64_t &free_event);
  void GetMaxStreamAndEventModel(uint32_t &max_stream_model, uint32_t &max_event_model);
  ///
  /// @ingroup domi_ome
  /// @brief delete model from model manager set
  ///
  Status DeleteModel(const uint32_t id);

  void GenModelId(uint32_t &id);

  static Status HandleDumpCommand(const Command &cmd_info);
  static Status HandleProfModelSubscribeCommand(const Command &cmd_info);
  static Status HandleProfModelUnsubscribeCommand(const Command &cmd_info);
  static Status HandleProfInitCommand(const Command &cmd_info);
  static Status HandleProfFinalizeCommand(const Command &cmd_info);
  static Status HandleProfStartCommand(const Command &cmd_info);
  static Status HandleProfStopCommand(const Command &cmd_info);

  static Status GetModelIdByCmd(const Command &cmd_info, uint32_t &model_id);

  Status LoadAsHeterogeneousModel(const uint32_t model_id,
                                  const FlowModelPtr &ge_root_model,
                                  const std::shared_ptr<ModelListener> &listener);

  bool UnloadHeterogeneousModel(const uint32_t model_id);

  Status ProfModelSubscribe(const uint64_t module, const uint32_t model_id, const uint32_t graph_id);

  void CreateMonitorThread();

  void RecordTsSnapshot();

  void ClearMonitorThread();

  void SetDumpProperties(std::shared_ptr<DavinciModel> davinci_model);

  static void getDevMsgCallback(const char_t *const msg, const uint32_t len);

  std::map<uint32_t, std::shared_ptr<DavinciModel>> model_map_;
  std::map<uint32_t, std::shared_ptr<hybrid::HybridDavinciModel>> hybrid_model_map_;
  std::map<uint32_t, std::shared_ptr<HeterogeneousModelExecutor>> heterogeneous_model_map_;
  std::map<std::string, std::vector<uint64_t>> model_aicpu_kernel_;
  uint32_t max_model_id_ = 0U;
  std::recursive_mutex map_mutex_;
  std::mutex session_id_create_mutex_;
  std::mutex exception_infos_mutex_;
  uint64_t session_id_bias_ = 0U;
  std::set<uint64_t> sess_ids_;
  std::vector<rtExceptionInfo> exception_infos_;
  std::mutex cust_aicpu_mutex_;
  std::map<uintptr_t, std::map<std::string, CustAICPUKernelPtr>> cust_aicpu_so_;

  std::mutex dump_properties_mutex_;
  DumpProperties dump_properties_;
  bool dump_exception_flag_ = false;
  bool is_socket_close_ = false;

  std::mutex monitor_mtx_;
  std::condition_variable monitor_cv_;
  std::thread monitor_thread_;
  bool stop_monitor_ = true;
  static std::string record_file_name_;
  std::string trigger_file_name_;
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_MODEL_MANAGER_H_
