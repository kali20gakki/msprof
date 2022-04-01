/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#ifndef FFTS_PLUS_FFTS_PLUS_UPDATE_H_
#define FFTS_PLUS_FFTS_PLUS_UPDATE_H_
#include "register/ffts_plus_task_update.h"
#include "register/ffts_plus_update_manager.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"
#include "common/fe_log.h"
#include "common/ffts_plus_type.h"
#include "runtime/rt_ffts_plus_define.h"
#include "runtime/rt_ffts_plus.h"

namespace fe {

  using ge::AutoThreadParam;
  using ge::AutoThreadSubTaskFlush;

class FFTSPlusFeUpdate : public ge::FFTSPlusTaskUpdate {
 public:
  FFTSPlusFeUpdate();
  ~FFTSPlusFeUpdate() override;
  // Interface to calculate auto thread param
  Status GetAutoThreadParam(const ge::NodePtr &node, const vector<optiling::utils::OpRunInfo> &op_run_info,
                            AutoThreadParam &auto_thread_param) override;
  // Interface to update AICore context
  Status UpdateSubTaskAndCache(const ge::NodePtr &node, const AutoThreadSubTaskFlush &flush_data,
                               rtFftsPlusTaskInfo_t &task_info) override;
 private:

  Status UpdateMixL2AicAivCtx(const rtFftsPlusTaskInfo_t &task_info,
                              const AutoThreadSubTaskFlush &flush_data, const ge::NodePtr node) const;
};

}

#endif
