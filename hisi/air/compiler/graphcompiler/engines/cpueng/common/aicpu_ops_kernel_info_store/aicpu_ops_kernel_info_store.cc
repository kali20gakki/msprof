/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#include "aicpu_ops_kernel_info_store.h"

#include "common/util/error_manager/error_manager.h"
#include "config/config_file.h"
#include "error_code/error_code.h"
#include "kernel_info.h"
#include "util/constant.h"
#include "util/log.h"
#include "util/util.h"

using namespace std;
using namespace ge;

namespace aicpu {

Status AicpuOpsKernelInfoStore::Initialize(const map<string, string> &options) {
  AICPUE_LOGI("Begin to initialize aicpu ops info store.")
  // check options
  auto iter = options.find(ge::SOC_VERSION);
  AICPU_IF_BOOL_EXEC(iter == options.end(),
      AICPU_REPORT_INNER_ERROR(
          "cannot find [%s] in param of aicpu op store initialize function.",
          ge::SOC_VERSION.c_str());
      return INPUT_PARAM_VALID)

  // lhisi not load TFKernel
  set<string> blacklist;
  if ((iter->second.find("Hi") != string::npos) ||
      (iter->second.find("SD") != string::npos)) {
    blacklist.insert("TFKernel");
  }

  AICPU_CHECK_RES(Finalize())

  string kernel_libs;
  string ops_kernel_config = Stringcat(engine_name_, "OpsKernel");
  if (!ConfigFile::GetInstance().GetValue(ops_kernel_config, kernel_libs)) {
    AICPU_REPORT_INNER_ERROR("[%s] not exist.", ops_kernel_config.c_str());
    return LOAD_PRIORITY_ITEM_FAILED;
  }

  // Parse string kernel_libs contains separator
  ConfigFile::GetInstance().SplitValue(kernel_libs, kernel_lib_names_, blacklist);
  if (kernel_lib_names_.empty()) {
    AICPUE_LOGW("kernelLibNames is empty.");
    return SUCCESS;
  }

  AICPUE_LOGI("First load [%s] Info Store", kernel_lib_names_[0].c_str())
  // Load all kernel libraries
  AICPU_CHECK_RES(LoadKernelLibs(options))
  // Get all op infos
  FillKernelInfos();
  AICPUE_LOGI("Aicpu kernel info store initialize success.");
  return SUCCESS;
}

Status AicpuOpsKernelInfoStore::Finalize() {
  kernel_libs_.clear();
  kernel_lib_names_.clear();
  op_infos_.clear();
  op_full_infos_.clear();
  return SUCCESS;
}

Status AicpuOpsKernelInfoStore::LoadKernelLibs(
    const map<string, string> &options) {
  for (const string &name : kernel_lib_names_) {
    FACTORY_KERNELINFO::FactoryType kernel_info_ptr =
        FACTORY_KERNELINFO::Produce(name);
    if (kernel_info_ptr == nullptr) {
      AICPU_REPORT_INNER_ERROR("create kernel in for store[%s] failed",
          name.c_str())
      return KERNELINFOSTORE_INSTANCE_FAILED;
    }
    Status flag = kernel_info_ptr->Initialize(options);
    if (flag != SUCCESS) {
      AICPU_REPORT_INNER_ERROR("kernel in for store[%s] initialize failed",
          name.c_str())
      return KERNELINFOSTORE_INITIALIZE_FAILED;
    }
    kernel_libs_[name] = kernel_info_ptr;
  }
  return SUCCESS;
}

void AicpuOpsKernelInfoStore::FillKernelInfos() {
  for (auto lib_iter = kernel_lib_names_.rbegin();
       lib_iter != kernel_lib_names_.rend(); ++lib_iter) {
    string kernel_name = *lib_iter;
    const KernelInfoPtr &kernel_lib_ptr = kernel_libs_[kernel_name];
    // how can be null...
    AICPU_IF_BOOL_EXEC(
        kernel_lib_ptr == nullptr,
        AICPU_REPORT_INNER_ERROR("kernel lib is nullptr, kernel name[%s].",
            kernel_name.c_str());
        return);
    map<string, OpFullInfo> op_full_infos;
    kernel_lib_ptr->GetOpInfos(op_full_infos);
    for (auto opIter = op_full_infos.cbegin(); opIter != op_full_infos.cend();
         ++opIter) {
      const string &op_type = opIter->first;
      const OpFullInfo &op_full_info = opIter->second;
      OpInfo op_info;
      op_info.engine = engine_name_;
      op_info.computeCost = op_full_info.computeCost;
      op_info.flagAsync = op_full_info.flagAsync;
      op_info.flagPartial = op_full_info.flagPartial;
      op_info.opKernelLib = op_full_info.opKernelLib;
      op_info.isAtomic = false;
      op_infos_[op_type] = op_info;
      op_full_infos_[op_type] = op_full_info;
    }
  }
}

void AicpuOpsKernelInfoStore::GetAllOpsKernelInfo(
    map<string, OpInfo> &infos) const {
  infos = op_infos_;
}

bool AicpuOpsKernelInfoStore::CheckSupported(const OpDescPtr &op_desc_ptr,
                                             string &unsupported_reason) const {
  AICPU_CHECK_NOTNULL_ERRCODE(op_desc_ptr, false);

  string op_type = op_desc_ptr->GetType();
  if (op_type.empty()) {
    AICPU_REPORT_INNER_ERROR("op type is empty, op[%s]",
        op_desc_ptr->GetName().c_str());
    return false;
  }

  // check whether the op is in aicpu ops kernel info store
  auto iter = op_full_infos_.find(op_type);
  if (iter == op_full_infos_.end()) {
    AICPUE_LOGEVENT("Internal kernel info store not include this op[%s].",
                    op_type.c_str());
    unsupported_reason = "Aicpu kernel info store not include this op ";
    unsupported_reason.append(op_type);
    return false;
  }

  auto op_full_info = iter->second;
  const map<string, string> data_types = op_full_info.inOutDataType;
  const map<string, string> in_out_real_name = op_full_info.inOutRealName;
  if (!op_desc_ptr->GetAllInputsDesc().empty()) {
    bool ret = CheckInputSupported(op_desc_ptr, data_types, in_out_real_name,
                                   unsupported_reason);
    AICPU_IF_BOOL_EXEC(
        !(ret), AICPUE_LOGEVENT("Failed to check input supported, op[%s].",
                                op_type.c_str());
        return false)
  }

  if (!op_desc_ptr->GetAllOutputsDesc().empty()) {
    bool ret = CheckOutputSupported(op_desc_ptr, data_types, in_out_real_name,
                                    unsupported_reason);
    AICPU_IF_BOOL_EXEC(
        !(ret), AICPUE_LOGEVENT("Failed to check output supported, op[%s].",
                                op_type.c_str());
        return false)
  }

  return true;
}

bool AicpuOpsKernelInfoStore::CheckInputSupported(
    const OpDescPtr &op_desc_ptr, const map<string, string> data_types,
    const map<string, string> in_out_real_name, string &unsupported_reason) const {
  uint32_t input_index = 0;
  for (const ge::GeTensorDesc input_desc : op_desc_ptr->GetAllInputsDesc()) {
    const string input_name = op_desc_ptr->GetInputNameByIndex(input_index);
    for (auto it = in_out_real_name.begin(); it != in_out_real_name.end(); it++) {
      const string input_real_name = it->first;
      if (input_name.compare(input_real_name) == 0) {
        const string data_type_name = it->second;
        set<DataType> dst_data_type;
        GetDataType(data_types, data_type_name, dst_data_type);
        const DataType type = input_desc.GetDataType();
        if (dst_data_type.find(type) == dst_data_type.end()) {
          string type_str;
          (void)ConvertDataType2String(type_str, type);
          string err_msg =
              Stringcat("data_type ", type_str, " of input ", input_real_name,
                        " is unsupported by current kernel info store. op type[",
                        op_desc_ptr->GetType(), "].");
          unsupported_reason = err_msg;
          AICPUE_LOGEVENT("The %s.", err_msg.c_str());
          return false;
        }
      }
    }
    input_index++;
  }

  return true;
}

bool AicpuOpsKernelInfoStore::CheckOutputSupported(
    const OpDescPtr &op_desc_ptr, const map<string, string> data_types,
    const map<string, string> in_out_real_name, string &unsupported_reason) const {
  uint32_t output_index = 0;
  for (const ge::GeTensorDesc output_desc : op_desc_ptr->GetAllOutputsDesc()) {
    const string output_name = op_desc_ptr->GetOutputNameByIndex(output_index);
    for (auto it = in_out_real_name.begin(); it != in_out_real_name.end(); it++) {
      const string output_real_name = it->first;
      if (output_name.compare(output_real_name) == 0) {
        const string data_type_name = it->second;
        set<DataType> dst_data_type;
        GetDataType(data_types, data_type_name, dst_data_type);
        const DataType type = output_desc.GetDataType();
        if (dst_data_type.find(type) == dst_data_type.end()) {
          string type_str;
          (void)ConvertDataType2String(type_str, type);
          string err_msg =
              Stringcat("dataType ", type_str, " of output ", output_real_name,
                        " is unsupported by current kernel info store. op type[",
                        op_desc_ptr->GetType(), "].");
          unsupported_reason = err_msg;
          AICPUE_LOGEVENT("The %s.", err_msg.c_str());
          return false;
        }
      }
    }
    output_index++;
  }
  return true;
}

void AicpuOpsKernelInfoStore::opsFlagCheck(const Node &node, string &ops_flag) {
  OpDescPtr op_desc_ptr = node.GetOpDesc();
  AICPU_IF_BOOL_EXEC(op_desc_ptr == nullptr,
      AICPU_REPORT_INNER_ERROR(
            "op desc is nullptr, op[%s].", node.GetName().c_str());
      return);
  string op_type = op_desc_ptr->GetType();
  auto iter = op_full_infos_.find(op_type);
  if (iter != op_full_infos_.end()) {
    ops_flag = (iter->second).opsFlag;
  } else {
    AICPUE_LOGW("Find [ops_flag] failed in this op type [%s]", op_type.c_str());
  }
  AICPUE_LOGI("Op[%s], ops_flag is [%s]", op_desc_ptr->GetName().c_str(),
              ops_flag.c_str());
}

void AicpuOpsKernelInfoStore::GetAllOpsFullKernelInfo(
    map<string, OpFullInfo> &infos) const {
  infos = op_full_infos_;
}

/**
 * For ops in AIcore, Call CompileOp before Generate task in AICPU
 * @param node_vec Node information
 * @return status whether operation successful
 */
ge::Status AicpuOpsKernelInfoStore::CompileOp(vector<ge::NodePtr> &node_vec) {
  if (node_vec.empty()) {
      AICPUE_LOGI("AicpuOpsKernelInfoStore's node_vec is empty in CompileOp.");
      return ge::SUCCESS;
  }

  AICPUE_LOGI("AicpuOpsKernelInfoStore's start CompileOp.");
  map<string, OpFullInfo> all_op_info;
  GetAllOpsFullKernelInfo(all_op_info);
  for (ge::NodePtr &node : node_vec) {
    AICPU_CHECK_NOTNULL(node)
    ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
    AICPU_CHECK_NOTNULL(op_desc_ptr)
    std::string op_type = op_desc_ptr->GetType();
    // check function op and framework op
    if ((op_type == kFunctionOp) || (op_type == kFrameworkOp)) {
      std::string err_msg = Stringcat("Can not create node def for function op and framework op[",
          node->GetName(), "] in CompileOp, op type[", op_type, "].");
      AICPU_REPORT_INNER_ERROR("%s.", err_msg.c_str());
      return ErrorCode::NODE_DEF_NOT_EXIST;
    }

    std::string kernel_name = GetKernelLibNameByOpType(op_type, all_op_info);
    auto iter = kernel_libs_.find(kernel_name);
    if (iter == kernel_libs_.end()) {
        AICPU_REPORT_INNER_ERROR("kernel lib[%s] is not exist.", kernel_name.c_str());
        return KERNEL_TYPE_INVALID;
    }
    const KernelInfoPtr &kernel_lib_ptr = iter->second;
    AICPU_CHECK_RES(kernel_lib_ptr->CompileOp(node));
  }

  AICPUE_LOGI("AicpuOpsKernelInfoStore's last Op run CompileOp Success.");
  return ge::SUCCESS;
}
}  // namespace aicpu
