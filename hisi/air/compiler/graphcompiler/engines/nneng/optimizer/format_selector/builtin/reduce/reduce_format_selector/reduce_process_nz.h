/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#ifndef FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_REDUCE_REDUCE_FORMAT_SELECTOR_REDUCE_PROCESS_NZ_H_
#define FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_REDUCE_REDUCE_FORMAT_SELECTOR_REDUCE_PROCESS_NZ_H_
#include "format_selector/builtin/reduce/reduce_format_selector/reduce_format_process.h"

namespace fe {

class ReduceProcessNz : public ReduceFormatProcess {
 public:
  ReduceProcessNz(){};

  ~ReduceProcessNz() override {};

  Status Process(const ge::OpDesc &op_desc, const FormatProccessArgs &args, FormatProccessResult &result) override;

  bool CheckOriginFormatAndShape(const vector<ge::Format> &formats, const vector<ge::GeShape> &shapes,
                                 const size_t &dim) override;

 private:
  // check if the last axis is C0 aligend and if the lastbutone is 16 aligned
  bool CheckNzAxisIsAligned(const ge::OpDesc &op_desc, const vector<ge::DataType> &dtypes,
                            const vector<ge::GeShape> &shapes, const bool &is_reduce_last,
                            const bool &is_reduce_lastbutone) const;

  bool CheckAxisIsC0Aligned(const ge::DataType &dtype, const int64_t &dim_value) const;
};

}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_FORMAT_SELECTOR_BUILTIN_REDUCE_REDUCE_FORMAT_SELECTOR_REDUCE_PROCESS_NZ_H_
