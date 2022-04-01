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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_FORMAT_AXIS_NAME_UTIL_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_FORMAT_AXIS_NAME_UTIL_H_

#include <functional>
#include <vector>
#include "common/fe_inner_error_codes.h"
#include "common/fe_utils.h"
#include "common/math_util.h"
#include "common/util/op_info_util.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"

namespace fe {
/* Axis value is arranged as {N,C,H,W,C1,C0,...} */
/* The first parameter is old shape's dimension,
 * second is c0 and third is axis value. */
using GetAxisNameByAxisValueInfo = std::function<Status(vector<int64_t> &, vector<std::string> &)>;

using GetAxisNameByAxisValueInfoPtr = std::shared_ptr<GetAxisNameByAxisValueInfo>;

class AxisNameUtil {
 public:
  static std::string GetReshapeType(const ge::Format &format, vector<int64_t> &axis_values);
  static std::vector<int64_t> GetExceptAxisValue(vector<int64_t> &axis_values, const size_t &axis_nums);
  /**
  * get axis name of NCHW by axis value
  * @param axis_values : axis value
  * @param axis_name : axis name
  * @return SUCCESS/FAILED
  */
  static std::string AxisNameToStr(std::vector<std::string> &axis_name);

  /**
   * set axis value for new format
   * @param op_desc : op desc info
   * @param origin_format : original format of current op
   * @param current_format : current format of op
   * @param origin_shape : original shape of current op
   * @return SUCCESS/FAILED
   */
  static Status GetNewAxisAttributeValue(const ge::OpDesc &op_desc, const ge::Format &origin_format,
                                         const ge::Format &current_format, const ge::GeShape &origin_shape,
                                         std::vector<int64_t> &axis_index_vec);
 private:
  /**
   * get all name except axis of NCHW by axis value
   * @param axis_values : axis value
   * @param axis_name : axis name
   * @return SUCCESS/FAILED
   */
  static Status GetNCHWAxisExceptName(std::vector<int64_t> &axis_values, std::vector<std::string> &axis_name);

  /**
   * get all name except axis of NHWC by axis value
   * @param axis_values : axis value
   * @param axis_name : axis name
   * @return SUCCESS/FAILED
   */
  static Status GetNHWCAxisExceptName(std::vector<int64_t> &axis_values, std::vector<std::string> &axis_name);

  /**
   * get all name except axis of HWCN by axis value
   * @param axis_values : axis value
   * @param axis_name : axis name
   * @return SUCCESS/FAILED
   */
  static Status GetHWCNAxisExceptName(std::vector<int64_t> &axis_values, std::vector<std::string> &axis_name);

  /**
   * get all name except axis of CHWN by axis value
   * @param axis_values : axis value
   * @param axis_name : axis name
   * @return SUCCESS/FAILED
   */
  static Status GetCHWNAxisExceptName(std::vector<int64_t> &axis_values, std::vector<std::string> &axis_name);

  /**
 * According to axis name, get the axis value.
 * @param op_desc : op desc info
 * @param format : format of current op
 * @param origin_shape : original shape of current op
 * @param axis_name : axis name(N/C/H/W)
 * @param axis_value : axis value to return
 * @return SUCCESS/FAILED
 */
  static Status GetNewAxisInfoByName(const ge::OpDesc &op_desc, const ge::Format &format,
                                     std::vector<std::string> &axis_name, std::vector<int64_t> &axis_index_vec);

  /**
   * According to axis value, get the axis name(N/C/H/W)
   * @param op_desc : op desc info
   * @param format : format of current op
   * @param origin_shape : original shape of current op
   * @param axis_name : axis name to return
   * @return SUCCESS/FAILED
   */
  static Status GetOriginalAxisName(const ge::OpDesc &op_desc, const ge::Format &format,
                                    const ge::GeShape &origin_shape, std::vector<std::string> &axis_name_vec);

  /**
   * get axis name of NCHW by axis value
   * @param axis_values : axis value
   * @param axis_name : axis name
   * @return SUCCESS/FAILED
   */
  static Status GetNCHWAxisName(std::vector<int64_t> &axis_values, std::vector<std::string> &axis_name);

  /**
   * get axis name of NHWC by axis value
   * @param axis_values : axis value
   * @param axis_name : axis name
   * @return SUCCESS/FAILED
   */
  static Status GetNHWCAxisName(std::vector<int64_t> &axis_values, std::vector<std::string> &axis_name);

  /**
   * get axis name of HWCN by axis value
   * @param axis_values : axis value
   * @param axis_name : axis name
   * @return SUCCESS/FAILED
   */
  static Status GetHWCNAxisName(std::vector<int64_t> &axis_values, std::vector<std::string> &axis_name);

  /**
   * get axis name of CHWN by axis value
   * @param axis_values : axis value
   * @param axis_name : axis name
   * @return SUCCESS/FAILED
   */
  static Status GetCHWNAxisName(std::vector<int64_t> &axis_values, std::vector<std::string> &axis_name);

  /**
   * get axis name of NDHWC by axis value
   * @param axis_values : axis value
   * @param axis_name : axis name
   * @return SUCCESS/FAILED
   */
  static Status GetNDHWCAxisName(std::vector<int64_t> &axis_values, std::vector<std::string> &axis_name);

  /**
   * get axis name of NCDHW by axis value
   * @param axis_values : axis value
   * @param axis_name : axis name
   * @return SUCCESS/FAILED
   */
  static Status GetNCDHWAxisName(std::vector<int64_t> &axis_values, std::vector<std::string> &axis_name);

  /**
 * get axis name of DHWCN by axis value
 * @param axis_values : axis value
 * @param axis_name : axis name
 * @return SUCCESS/FAILED
 */
  static Status GetDHWCNAxisName(std::vector<int64_t> &axis_values, std::vector<std::string> &axis_name);

  /* map of GetAxisValueInfoByFormat, get axis value by different original
   * formats. */
  static const std::map<ge::Format, GetAxisNameByAxisValueInfoPtr> get_axis_name_except_func_map;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_FORMAT_AXIS_NAME_UTIL_H_
