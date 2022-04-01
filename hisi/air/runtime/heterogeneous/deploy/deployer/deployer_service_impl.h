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

#ifndef AIR_RUNTIME_DEPLOY_DAEMON_DEPLOYER_SERVICE_IMPL_H_
#define AIR_RUNTIME_DEPLOY_DAEMON_DEPLOYER_SERVICE_IMPL_H_

#include <map>
#include <string>
#include <vector>
#include "ge/ge_api_error_codes.h"
#include "common/config/json_parser.h"
#include "deploy/deployer/deploy_context.h"
#include "proto/deployer.pb.h"

namespace ge {
class DeployerServiceImpl {
 public:
  static DeployerServiceImpl &GetInstance() {
    static DeployerServiceImpl instance;
    return instance;
  }
  /*
   *  @ingroup ge
   *  @brief   deployer service interface
   *  @param   [in]  context        grpc context
   *  @param   [in]  request        service request
   *  @param   [out] response       service response
   *  @return  SUCCESS or FAILED
   */
  Status Process(DeployContext &context,
                 const deployer::DeployerRequest &request,
                 deployer::DeployerResponse &response);

  /*
   *  @ingroup ge
   *  @brief   deployer service interface
   *  @param   [in]  context        grpc context
   *  @param   [in]  request        service request
   *  @param   [out] response       service response
   *  @return  SUCCESS or FAILED
   */
  using ProcessFunc = std::function<void(DeployContext &context,
                                         const deployer::DeployerRequest &request,
                                         deployer::DeployerResponse &response)>;

  /*
  *  @ingroup ge
  *  @brief   register deployer server api
  *  @param   [in]  DeployerRequestType
  *  @param   [in]  const ProcessFunc &
  *  @return: None
   *
  */
  void RegisterReqProcessor(deployer::DeployerRequestType type, const ProcessFunc &fn);

  /*
   *  @ingroup ge
   *  @brief   pre-download model
   *  @param   [in]  context        grpc context
   *  @param   [in]  request        service request
   *  @param   [out] response       service response
   *  @return: None
   */
  static void PreDeployModelProcess(DeployContext &context,
                                    const deployer::DeployerRequest &request,
                                    deployer::DeployerResponse &response);

  /*
   *  @ingroup ge
   *  @brief   pre download model
   *  @param   [in]  context        grpc context
   *  @param   [in]  request        service request
   *  @param   [out] response       service response
   *  @return: None
   */
  static void PreDownloadModelProcess(DeployContext &context,
                                      const deployer::DeployerRequest &request,
                                      deployer::DeployerResponse &response);

  /*
   *  @ingroup ge
   *  @brief   download device maintenance config
   *  @param   [in]  context        grpc context
   *  @param   [in]  request        service request
   *  @param   [out] response       service response
   *  @return: None
   */
  static void DownloadDevMaintenanceCfgProcess(DeployContext &context,
                                               const deployer::DeployerRequest &request,
                                               deployer::DeployerResponse &response);
  /*
   *  @ingroup ge
   *  @brief   download model
   *  @param   [in]  context        grpc context
   *  @param   [in]  request        service request
   *  @param   [out] response       service response
   *  @return: None
   */
  static void DownloadModelProcess(DeployContext &context,
                                   const deployer::DeployerRequest &request,
                                   deployer::DeployerResponse &response);

  /*
   *  @ingroup ge
   *  @brief   load model
   *  @param   [in]  context        grpc context
   *  @param   [in]  request        service request
   *  @param   [out] response       service response
   *  @return: None
   */
  static void LoadModelProcess(DeployContext &context,
                               const deployer::DeployerRequest &request,
                               deployer::DeployerResponse &response);

  /*
   *  @ingroup ge
   *  @brief   unload model
   *  @param   [in]  context        grpc context
   *  @param   [in]  request        service request
   *  @param   [out] response       service response
   *  @return: None
   */
  static void UnloadModelProcess(DeployContext &context,
                                 const deployer::DeployerRequest &request,
                                 deployer::DeployerResponse &response);

  static void MultiVarManagerInfoProcess(DeployContext &context,
                                         const deployer::DeployerRequest &request,
                                         deployer::DeployerResponse &response);

  static void SharedContentProcess(DeployContext &context,
                                   const deployer::DeployerRequest &request,
                                   deployer::DeployerResponse &response);

  static void DeployRankTableProcess(DeployContext &context,
                                     const deployer::DeployerRequest &request,
                                     deployer::DeployerResponse &response);

 private:
  std::map<deployer::DeployerRequestType, ProcessFunc> process_fns_;
  std::mutex mu_;
};
}  // namespace ge

#endif  // AIR_RUNTIME_DEPLOY_DAEMON_DEPLOYER_SERVICE_IMPL_H_
