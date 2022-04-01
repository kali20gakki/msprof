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

#include "transfer_shape_according_to_format.h"
#include "common/fe_utils.h"

namespace fe {
const std::map<ge::Format, GetNewShapeByAxisValueAndFormatPtr> ShapeTransferAccordingToFormat::get_new_shape_func_map =
    {{ge::FORMAT_NCHW, std::make_shared<GetNewShapeByAxisValueAndFormat>(GetNCHWShapeByAxisValue)},
     {ge::FORMAT_NHWC, std::make_shared<GetNewShapeByAxisValueAndFormat>(GetNHWCShapeByAxisValue)},
     {ge::FORMAT_NC1HWC0, std::make_shared<GetNewShapeByAxisValueAndFormat>(GetNC1HWC0ShapeByAxisValue)},
     {ge::FORMAT_NC1HWC0_C04, std::make_shared<GetNewShapeByAxisValueAndFormat>(GetNC1HWC0ShapeByAxisValue)},
     {ge::FORMAT_FRACTAL_Z, std::make_shared<GetNewShapeByAxisValueAndFormat>(GetFzShapeByAxisValue)},
     {ge::FORMAT_FRACTAL_Z_C04, std::make_shared<GetNewShapeByAxisValueAndFormat>(GetFzC04ShapeByAxisValue)},
     {ge::FORMAT_HWCN, std::make_shared<GetNewShapeByAxisValueAndFormat>(GetHWCNShapeByAxisValue)},
     {ge::FORMAT_C1HWNCoC0, std::make_shared<GetNewShapeByAxisValueAndFormat>(GetC1HWNCoC0ShapeByAxisValue)},
     {ge::FORMAT_CHWN, std::make_shared<GetNewShapeByAxisValueAndFormat>(GetCHWNShapeByAxisValue)},
     {ge::FORMAT_FRACTAL_NZ, std::make_shared<GetNewShapeByAxisValueAndFormat>(GetNzShapeByAxisValue)},
     {ge::FORMAT_FRACTAL_Z_3D, std::make_shared<GetNewShapeByAxisValueAndFormat>(GetFz3DShapeByAxisValue)},
     {ge::FORMAT_FRACTAL_Z_3D_TRANSPOSE,
      std::make_shared<GetNewShapeByAxisValueAndFormat>(GetFz3DTransposeShapeByAxisValue)},
     {ge::FORMAT_NDC1HWC0, std::make_shared<GetNewShapeByAxisValueAndFormat>(GetNDC1HWC0ShapeByAxisValue)},
     {ge::FORMAT_FRACTAL_ZN_LSTM, std::make_shared<GetNewShapeByAxisValueAndFormat>(GetFzLstmShapeByAxisValue)},
     {ge::FORMAT_FRACTAL_ZN_RNN, std::make_shared<GetNewShapeByAxisValueAndFormat>(GetFznRNNShapeByAxisValue)},
     {ge::FORMAT_ND_RNN_BIAS, std::make_shared<GetNewShapeByAxisValueAndFormat>(GetNDRNNShapeByAxisValue)}};

Status ShapeTransferAccordingToFormat::GetNCHWShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                                               const vector<int64_t> &axis_value,
                                                               const vector<int64_t> &nd_value) {
  if (axis_value.empty()) {
    FE_LOGW("AxisValue is empty!");
    return SUCCESS;
  }
  /* axis_value is initialized as a size 6 vector. */
  std::vector<int64_t> new_dim_vec;
  new_dim_vec.push_back(axis_value[AXIS_N]);
  new_dim_vec.push_back(axis_value[AXIS_C]);
  new_dim_vec.push_back(axis_value[AXIS_H]);
  new_dim_vec.push_back(axis_value[AXIS_W]);
  new_shape = ge::GeShape(new_dim_vec);
  return SUCCESS;
}

Status ShapeTransferAccordingToFormat::GetNHWCShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                                               const vector<int64_t> &axis_value,
                                                               const vector<int64_t> &nd_value) {
  if (axis_value.empty()) {
    FE_LOGW("AxisValue is empty!");
    return SUCCESS;
  }
  /* axis_value is initialized as a size 6 vector. */
  std::vector<int64_t> new_dim_vec;
  new_dim_vec.push_back(axis_value[AXIS_N]);
  new_dim_vec.push_back(axis_value[AXIS_H]);
  new_dim_vec.push_back(axis_value[AXIS_W]);
  new_dim_vec.push_back(axis_value[AXIS_C]);
  new_shape = ge::GeShape(new_dim_vec);
  return SUCCESS;
}

Status ShapeTransferAccordingToFormat::GetNC1HWC0ShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                                                  const vector<int64_t> &axis_value,
                                                                  const vector<int64_t> &nd_value) {
  if (axis_value.empty()) {
    FE_LOGW("AxisValue is empty!");
    return SUCCESS;
  }
  /* axis_value is initialized as a size 6 vector. */
  std::vector<int64_t> new_dim_vec;
  if (impl_type == EN_IMPL_HW_TBE || impl_type == EN_IMPL_CUSTOM_TBE ||
      impl_type == EN_IMPL_NON_PERSISTENT_CUSTOM_TBE) {
    new_dim_vec.push_back(axis_value[AXIS_N]);
    new_dim_vec.push_back(axis_value[AXIS_C1]);
    new_dim_vec.push_back(axis_value[AXIS_H]);
    new_dim_vec.push_back(axis_value[AXIS_W]);
    new_dim_vec.push_back(axis_value[AXIS_C0]);
    new_shape = ge::GeShape(new_dim_vec);
  } else {
    new_dim_vec.push_back(axis_value[AXIS_N]);
    new_dim_vec.push_back(axis_value[AXIS_C]);
    new_dim_vec.push_back(axis_value[AXIS_H]);
    new_dim_vec.push_back(axis_value[AXIS_W]);
    new_shape = ge::GeShape(new_dim_vec);
  }
  return SUCCESS;
}

Status ShapeTransferAccordingToFormat::GetFzShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                                             const vector<int64_t> &axis_value,
                                                             const vector<int64_t> &nd_value) {
  if (axis_value.empty()) {
    FE_LOGW("AxisValue is empty!");
    return SUCCESS;
  }
  /* axis_value is initialized as a size 6 vector. */
  std::vector<int64_t> new_dim_vec;
  if (nd_value.size() == SIZE_OF_CN) {
    auto size_of_original_vec = nd_value.size();
    new_dim_vec = nd_value;
    /* size_of_original_vec - 1 mean the last value of original vec
     * size_of_original_vec - 2 mean the second last value of original vec */
    new_dim_vec[size_of_original_vec - MINUS_VALUE_ONE] =
        DivisionCeiling(nd_value[size_of_original_vec - MINUS_VALUE_ONE], SHAPE_NUMBER_16);
    new_dim_vec[size_of_original_vec - MINUS_VALUE_TWO] =
        DivisionCeiling(nd_value[size_of_original_vec - MINUS_VALUE_TWO], axis_value[AXIS_C0]);
    new_dim_vec.push_back(SHAPE_NUMBER_16);
    new_dim_vec.push_back(axis_value[AXIS_C0]);
    new_shape = ge::GeShape(new_dim_vec);
  } else {
    if (impl_type == EN_IMPL_HW_TBE || impl_type == EN_IMPL_CUSTOM_TBE ||
        impl_type == EN_IMPL_NON_PERSISTENT_CUSTOM_TBE) {
      bool has_unknown_shape = axis_value[AXIS_W] == UNKNOWN_SHAPE_VALUE || axis_value[AXIS_H] == UNKNOWN_SHAPE_VALUE ||
                               axis_value[AXIS_C1] == UNKNOWN_SHAPE_VALUE || axis_value[AXIS_G] == UNKNOWN_SHAPE_VALUE;
      int64_t hwc1 = UNKNOWN_SHAPE_VALUE;
      int64_t group_val = axis_value[AXIS_G];
      int64_t axis_g_val = GROUPS_DEFAULT_VALUE;
      int64_t axis_n_val = axis_value[AXIS_N];
      int64_t axis_c_val = axis_value[AXIS_C];
      int64_t axis_c1_val = axis_value[AXIS_C1];
      if (!has_unknown_shape) {
        if (group_val > GROUPS_DEFAULT_VALUE && axis_n_val >= group_val) {
          int64_t enlarge_value =
              GetAsisEnlargeValue(axis_c_val, axis_n_val / group_val, axis_value[AXIS_C0], group_val);
          axis_g_val = DivisionCeiling(group_val, enlarge_value);
          FE_INT64_MULCHECK(axis_c_val, enlarge_value);
          axis_c_val *= enlarge_value;
          FE_INT64_MULCHECK(axis_n_val / group_val, enlarge_value);
          axis_n_val = (axis_n_val / group_val) * enlarge_value;
          axis_c1_val = DivisionCeiling(axis_c_val, axis_value[AXIS_C0]);
        }
        FE_INT64_MULCHECK(axis_g_val, axis_c1_val);
        int64_t g_c1_val = axis_g_val * axis_c1_val;
        FE_INT64_MULCHECK(g_c1_val, axis_value[AXIS_H]);
        g_c1_val *= axis_value[AXIS_H];
        FE_INT64_MULCHECK(g_c1_val, axis_value[AXIS_W]);
        hwc1 = g_c1_val * axis_value[AXIS_W];
      }

      new_dim_vec.push_back(hwc1);
      new_dim_vec.push_back(DivisionCeiling(axis_n_val, NI));
      new_dim_vec.push_back(NI);
      new_dim_vec.push_back(axis_value[AXIS_C0]);
      new_shape = ge::GeShape(new_dim_vec);
    } else {
      new_dim_vec.push_back(axis_value[AXIS_N]);
      new_dim_vec.push_back(axis_value[AXIS_C]);
      new_dim_vec.push_back(axis_value[AXIS_H]);
      new_dim_vec.push_back(axis_value[AXIS_W]);
      new_shape = ge::GeShape(new_dim_vec);
    }
  }
  return SUCCESS;
}

Status ShapeTransferAccordingToFormat::GetFzC04ShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                                                const vector<int64_t> &axis_value,
                                                                const vector<int64_t> &nd_value) {
  if (axis_value.empty()) {
    FE_LOGW("AxisValue is empty!");
    return SUCCESS;
  }
  /* axis_value is initialized as a size 6 vector. */
  std::vector<int64_t> new_dim_vec;
  if (impl_type == EN_IMPL_HW_TBE || impl_type == EN_IMPL_CUSTOM_TBE) {
    if (axis_value[AXIS_H] == UNKNOWN_SHAPE_VALUE || axis_value[AXIS_W] == UNKNOWN_SHAPE_VALUE) {
      new_dim_vec.push_back(UNKNOWN_SHAPE_VALUE);
    } else {
      FE_INT64_MULCHECK(SHAPE_DIM_VALUE_C04, axis_value[AXIS_H]);
      int64_t x = SHAPE_DIM_VALUE_C04 * axis_value[AXIS_H];
      FE_INT64_MULCHECK(x, axis_value[AXIS_W]);
      x *= axis_value[AXIS_W];
      new_dim_vec.push_back(DivisionCeiling(x, X0));
    }

    new_dim_vec.push_back(DivisionCeiling(axis_value[AXIS_N], NI));
    new_dim_vec.push_back(NI);
    new_dim_vec.push_back(X0);
    new_shape = ge::GeShape(new_dim_vec);
  } else {
    new_dim_vec.push_back(axis_value[AXIS_N]);
    new_dim_vec.push_back(axis_value[AXIS_C]);
    new_dim_vec.push_back(axis_value[AXIS_H]);
    new_dim_vec.push_back(axis_value[AXIS_W]);
    new_shape = ge::GeShape(new_dim_vec);
  }
  return SUCCESS;
}

Status ShapeTransferAccordingToFormat::GetHWCNShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                                               const vector<int64_t> &axis_value,
                                                               const vector<int64_t> &nd_value) {
  if (axis_value.empty()) {
    FE_LOGW("AxisValue is empty!");
    return SUCCESS;
  }
  /* axis_value is initialized as a size 6 vector. */
  std::vector<int64_t> new_dim_vec;
  new_dim_vec.push_back(axis_value[AXIS_H]);
  new_dim_vec.push_back(axis_value[AXIS_W]);
  new_dim_vec.push_back(axis_value[AXIS_C]);
  new_dim_vec.push_back(axis_value[AXIS_N]);
  new_shape = ge::GeShape(new_dim_vec);
  return SUCCESS;
}

Status ShapeTransferAccordingToFormat::GetCHWNShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                                               const vector<int64_t> &axis_value,
                                                               const vector<int64_t> &nd_value) {
  if (axis_value.empty()) {
    FE_LOGW("AxisValue is empty!");
    return SUCCESS;
  }
  /* axis_value is initialized as a size 6 vector. */
  std::vector<int64_t> new_dim_vec;
  new_dim_vec.push_back(axis_value[AXIS_C]);
  new_dim_vec.push_back(axis_value[AXIS_H]);
  new_dim_vec.push_back(axis_value[AXIS_W]);
  new_dim_vec.push_back(axis_value[AXIS_N]);
  new_shape = ge::GeShape(new_dim_vec);
  return SUCCESS;
}

Status ShapeTransferAccordingToFormat::GetC1HWNCoC0ShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                                                    const vector<int64_t> &axis_value,
                                                                    const vector<int64_t> &nd_value) {
  if (axis_value.empty()) {
    FE_LOGW("AxisValue is empty!");
    return SUCCESS;
  }
  std::vector<int64_t> new_dim_vec;
  new_dim_vec.push_back(axis_value[AXIS_C1]);
  new_dim_vec.push_back(axis_value[AXIS_H]);
  new_dim_vec.push_back(axis_value[AXIS_W]);
  new_dim_vec.push_back(DivisionCeiling(axis_value[AXIS_N], 16));
  new_dim_vec.push_back(axis_value[AXIS_Co]);
  new_dim_vec.push_back(axis_value[AXIS_C0]);
  new_shape = ge::GeShape(new_dim_vec);
  return SUCCESS;
}

Status ShapeTransferAccordingToFormat::GetNzShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                                             const vector<int64_t> &axis_value,
                                                             const vector<int64_t> &nd_value) {
  if (nd_value.empty()) {
    FE_LOGW("ndValue is empty!");
    return SUCCESS;
  }
  if (axis_value.empty() || axis_value.size() <= AXIS_C0) {
    FE_LOGW("AxisValue is empty or its size %zu <= AXIS_C0[%u]", axis_value.size(), AXIS_C0);
    return SUCCESS;
  }
  uint32_t size_of_original_vec = nd_value.size();
  std::vector<int64_t> nd_value_temp = nd_value;
  if (size_of_original_vec < MINIMUM_NZ_SHAPE_DIM_NUM) {
    FE_LOGW("ndValue's dim num is less than 2, insert 1 at tail!");
    nd_value_temp.push_back(1);
    size_of_original_vec++;
  }
  std::vector<int64_t> new_dim_vec = nd_value_temp;

  /* size_of_original_vec - 1 mean the last value of original vec
   * size_of_original_vec - 2 mean the second last value of original vec */
  new_dim_vec[size_of_original_vec - MINUS_VALUE_ONE] =
      DivisionCeiling(nd_value_temp[size_of_original_vec - MINUS_VALUE_TWO], static_cast<int64_t>(SHAPE_NUMBER_16));
  new_dim_vec[size_of_original_vec - MINUS_VALUE_TWO] =
      DivisionCeiling(nd_value_temp[size_of_original_vec - MINUS_VALUE_ONE], axis_value[AXIS_C0]);

  new_dim_vec.push_back(SHAPE_NUMBER_16);
  new_dim_vec.push_back(axis_value[AXIS_C0]);
  new_shape = ge::GeShape(new_dim_vec);
  return SUCCESS;
}

Status ShapeTransferAccordingToFormat::GetNDC1HWC0ShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                                                   const vector<int64_t> &axis_value,
                                                                   const vector<int64_t> &nd_value) {
  if (axis_value.empty()) {
    FE_LOGW("AxisValue is empty!");
    return SUCCESS;
  }
  /* axis_value is initialized as a size 6 vector. */
  std::vector<int64_t> new_dim_vec;
  new_dim_vec.push_back(axis_value[AXIS_N]);
  new_dim_vec.push_back(axis_value[AXIS_D]);
  new_dim_vec.push_back(axis_value[AXIS_C1]);
  new_dim_vec.push_back(axis_value[AXIS_H]);
  new_dim_vec.push_back(axis_value[AXIS_W]);
  new_dim_vec.push_back(axis_value[AXIS_C0]);
  new_shape = ge::GeShape(new_dim_vec);
  return SUCCESS;
}

Status ShapeTransferAccordingToFormat::GetFz3DShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                                               const vector<int64_t> &axis_value,
                                                               const vector<int64_t> &nd_value) {
  if (axis_value.empty()) {
    FE_LOGW("AxisValue is empty!");
    return SUCCESS;
  }

  /* axis_value is initialized as a size 6 vector. */
  bool has_unknown_shape = axis_value[AXIS_D] == UNKNOWN_SHAPE_VALUE || axis_value[AXIS_H] == UNKNOWN_SHAPE_VALUE ||
                           axis_value[AXIS_W] == UNKNOWN_SHAPE_VALUE || axis_value[AXIS_C1] == UNKNOWN_SHAPE_VALUE ||
                           axis_value[AXIS_G] == UNKNOWN_SHAPE_VALUE;

  int64_t gdhwc1 = UNKNOWN_SHAPE_VALUE;
  int64_t group_val = axis_value[AXIS_G];
  int64_t axis_g_val = GROUPS_DEFAULT_VALUE;
  int64_t axis_n_val = axis_value[AXIS_N];
  int64_t axis_c_val = axis_value[AXIS_C];
  int64_t axis_c1_val = axis_value[AXIS_C1];
  if (!has_unknown_shape) {
    if (group_val > GROUPS_DEFAULT_VALUE && axis_n_val >= group_val) {
      int64_t enlarge_value = GetAsisEnlargeValue(axis_c_val, axis_n_val / group_val,
                                                  axis_value[AXIS_C0], group_val);
      axis_g_val = DivisionCeiling(group_val, enlarge_value);
      FE_INT64_MULCHECK(axis_c_val, enlarge_value);
      axis_c_val *= enlarge_value;
      FE_INT64_MULCHECK(axis_n_val / group_val, enlarge_value);
      axis_n_val = (axis_n_val / group_val) * enlarge_value;
      axis_c1_val = DivisionCeiling(axis_c_val, axis_value[AXIS_C0]);
    }
    FE_INT64_MULCHECK(axis_g_val, axis_c1_val);
    int64_t g_c1_val = axis_g_val * axis_c1_val;
    FE_INT64_MULCHECK(g_c1_val, axis_value[AXIS_D]);
    g_c1_val *= axis_value[AXIS_D];
    FE_INT64_MULCHECK(g_c1_val, axis_value[AXIS_H]);
    g_c1_val *= axis_value[AXIS_H];
    FE_INT64_MULCHECK(g_c1_val, axis_value[AXIS_W]);
    gdhwc1 = g_c1_val * axis_value[AXIS_W];
  }
  std::vector<int64_t> new_dim_vec;
  new_dim_vec.push_back(gdhwc1);
  new_dim_vec.push_back(DivisionCeiling(axis_n_val, NI));
  new_dim_vec.push_back(NI);
  new_dim_vec.push_back(axis_value[AXIS_C0]);
  new_shape = ge::GeShape(new_dim_vec);

  return SUCCESS;
}

Status ShapeTransferAccordingToFormat::GetFz3DTransposeShapeByAxisValue(ge::GeShape &new_shape,
                                                                        const int64_t &impl_type,
                                                                        const vector<int64_t> &axis_value,
                                                                        const vector<int64_t> &nd_value) {
  if (axis_value.empty()) {
    FE_LOGW("AxisValue is empty!");
    return SUCCESS;
  }
  /* axis_value is initialized as a size 6 vector. */
  std::vector<int64_t> new_dim_vec;
  for (auto ele : axis_value) {
    FE_LOGI("value is %ld", ele);
  }
  int64_t n1 = DivisionCeiling(axis_value[AXIS_N], NI);
  FE_INT64_MULCHECK(n1, axis_value[AXIS_H]);
  int64_t dhwn1 = n1 * axis_value[AXIS_H];
  FE_INT64_MULCHECK(dhwn1, axis_value[AXIS_W]);
  dhwn1 *= axis_value[AXIS_W];
  FE_INT64_MULCHECK(dhwn1, axis_value[AXIS_D]);
  dhwn1 *= axis_value[AXIS_D];
  if (n1 == UNKNOWN_SHAPE_VALUE || axis_value[AXIS_H] == UNKNOWN_SHAPE_VALUE ||
      axis_value[AXIS_W] == UNKNOWN_SHAPE_VALUE || axis_value[AXIS_D] == UNKNOWN_SHAPE_VALUE) {
    dhwn1 = UNKNOWN_SHAPE_VALUE;
  }

  new_dim_vec.push_back(dhwn1);
  if (axis_value[AXIS_C] == UNKNOWN_SHAPE_VALUE) {
    new_dim_vec.push_back(UNKNOWN_SHAPE_VALUE);
  } else {
    new_dim_vec.push_back(axis_value[AXIS_C1]);
  }
  new_dim_vec.push_back(NI);
  new_dim_vec.push_back(axis_value[AXIS_C0]);
  new_shape = ge::GeShape(new_dim_vec);

  return SUCCESS;
}

Status ShapeTransferAccordingToFormat::GetFzLstmShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                                                 const vector<int64_t> &axis_value,
                                                                 const vector<int64_t> &nd_value) {
  if (axis_value.empty()) {
    FE_LOGW("AxisValue is empty!");
    return SUCCESS;
  }
  /* axis_value is initialized as a size 6 vector. */
  std::vector<int64_t> new_dim_vec;
  int64_t h = axis_value[AXIS_N] / LSTM_NI;
  FE_INT64_SUBCHECK(axis_value[AXIS_C], h);
  int64_t i = axis_value[AXIS_C] - h;
  FE_INT64_ADDCHECK(DivisionCeiling(i, NI), DivisionCeiling(h, NI));
  int64_t first_element_of_fz_lstm = DivisionCeiling(i, NI) + DivisionCeiling(h, NI);

  int64_t second_element_of_fz_lstm = LSTM_NI * DivisionCeiling(h, NI);
  if (axis_value[AXIS_N] == UNKNOWN_SHAPE_VALUE || axis_value[AXIS_C] == UNKNOWN_SHAPE_VALUE) {
    first_element_of_fz_lstm = UNKNOWN_SHAPE_VALUE;
    second_element_of_fz_lstm = UNKNOWN_SHAPE_VALUE;
  }
  new_dim_vec.push_back(first_element_of_fz_lstm);
  new_dim_vec.push_back(second_element_of_fz_lstm);
  new_dim_vec.push_back(NI);
  new_dim_vec.push_back(NI);
  new_shape = ge::GeShape(new_dim_vec);
  return SUCCESS;
}

Status ShapeTransferAccordingToFormat::GetNewDimVec(ge::GeShape &new_shape, const int64_t &impl_type,
                                                    const vector<int64_t> &axis_value,
                                                    const vector<int64_t> &nd_value) {
  uint32_t size_of_original_vec = nd_value.size();
  if (size_of_original_vec < MINIMUM_ND_TO_RNN_SHAPE_NUM) {
    FE_LOGW("ndValue's dim num is less than 2!");
    return SUCCESS;
  }
  /* check nd shape value */
  int64_t k_num;
  int64_t n_num;
  int64_t k_value = nd_value[size_of_original_vec - MINUS_VALUE_TWO];
  int64_t hidden_or_state_size = axis_value[AXIS_HIDEEN_SIZE];
  if (axis_value[AXIS_STATE_SIZE] != RNN_STATE_SIZE_DEFAULT_VALUE) {
    hidden_or_state_size = axis_value[AXIS_STATE_SIZE];
  }
  FE_INT64_ADDCHECK(hidden_or_state_size, axis_value[AXIS_INPUT_SIZE]);
  if (k_value == hidden_or_state_size + axis_value[AXIS_INPUT_SIZE]) {
    k_num = 2;
  } else if (k_value == hidden_or_state_size || k_value == axis_value[AXIS_INPUT_SIZE]) {
    k_num = 1;
  } else {
    REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][FznRNN] The second-last dim value of shape is not valid.");
    return SUCCESS;
  }
  int64_t n_value = nd_value[size_of_original_vec - MINUS_VALUE_ONE];
  FE_INT64_ZEROCHECK(axis_value[AXIS_HIDEEN_SIZE]);
  if (n_value % axis_value[AXIS_HIDEEN_SIZE] != 0) {
    REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][FznRNN] The last dim value of shape is not valid.");
    return SUCCESS;
  }
  n_num = n_value / axis_value[AXIS_HIDEEN_SIZE];

  /* axis_value is initialized as a size 6 vector. */
  std::vector<int64_t> new_dim_vec = nd_value;

  if (k_num == 1) {
    new_dim_vec[size_of_original_vec - MINUS_VALUE_TWO] =
            DivisionCeiling(k_value, static_cast<int64_t>(SHAPE_NUMBER_16));
  } else {
    FE_INT64_ADDCHECK(DivisionCeiling(axis_value[AXIS_INPUT_SIZE], static_cast<int64_t>(SHAPE_NUMBER_16)),
                      DivisionCeiling(hidden_or_state_size, static_cast<int64_t>(SHAPE_NUMBER_16)));
    new_dim_vec[size_of_original_vec - MINUS_VALUE_TWO] =
            DivisionCeiling(axis_value[AXIS_INPUT_SIZE], static_cast<int64_t>(SHAPE_NUMBER_16)) +
            DivisionCeiling(hidden_or_state_size, static_cast<int64_t>(SHAPE_NUMBER_16));
  }
  FE_INT64_MULCHECK(n_num, DivisionCeiling(axis_value[AXIS_HIDEEN_SIZE], axis_value[AXIS_C0]));
  new_dim_vec[size_of_original_vec - MINUS_VALUE_ONE] =
          n_num * DivisionCeiling(axis_value[AXIS_HIDEEN_SIZE], axis_value[AXIS_C0]);

  new_dim_vec.push_back(SHAPE_NUMBER_16);
  new_dim_vec.push_back(axis_value[AXIS_C0]);
  new_shape = ge::GeShape(new_dim_vec);
  return SUCCESS;
}

Status ShapeTransferAccordingToFormat::GetFznRNNShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                                                 const vector<int64_t> &axis_value,
                                                                 const vector<int64_t> &nd_value) {
  if (nd_value.empty()) {
    FE_LOGW("ndValue is empty!");
    return SUCCESS;
  }
  if (axis_value.empty() || axis_value.size() <= AXIS_STATE_SIZE) {
    FE_LOGW("AxisValue is empty or its size %zu <= AXIS_STATE_SIZE[%u]", axis_value.size(), AXIS_STATE_SIZE);
    return SUCCESS;
  }
  Status res = GetNewDimVec(new_shape, impl_type, axis_value, nd_value);
  return res;
}

Status ShapeTransferAccordingToFormat::GetNDRNNShapeByAxisValue(ge::GeShape &new_shape, const int64_t &impl_type,
                                                                const vector<int64_t> &axis_value,
                                                                const vector<int64_t> &nd_value) {
  if (nd_value.empty()) {
    FE_LOGW("ndValue is empty!");
    return SUCCESS;
  }
  if (axis_value.empty() || axis_value.size() <= AXIS_HIDEEN_SIZE) {
    FE_LOGW("AxisValue is empty or its size %zu <= AXIS_HIDEEN_SIZE[%u]", axis_value.size(), AXIS_HIDEEN_SIZE);
    return SUCCESS;
  }
  uint32_t size_of_original_vec = nd_value.size();

  /* check nd shape value */
  int64_t n_num;
  int64_t n_value = nd_value[size_of_original_vec - MINUS_VALUE_ONE];
  FE_INT64_ZEROCHECK(axis_value[AXIS_HIDEEN_SIZE]);
  if (n_value % axis_value[AXIS_HIDEEN_SIZE] != 0) {
    FE_LOGW("The last dim value of shape is not valid.");
    return SUCCESS;
  }
  n_num = n_value / axis_value[AXIS_HIDEEN_SIZE];

  /* axis_value is initialized as a size 6 vector. */
  std::vector<int64_t> new_dim_vec = nd_value;
  FE_INT64_MULCHECK(n_num, DivisionCeiling(axis_value[AXIS_HIDEEN_SIZE], axis_value[AXIS_C0]));
  FE_INT64_MULCHECK(n_num * DivisionCeiling(axis_value[AXIS_HIDEEN_SIZE], axis_value[AXIS_C0]), axis_value[AXIS_C0]);
  new_dim_vec[size_of_original_vec - MINUS_VALUE_ONE] =
          n_num * DivisionCeiling(axis_value[AXIS_HIDEEN_SIZE], axis_value[AXIS_C0]) * axis_value[AXIS_C0];
  new_shape = ge::GeShape(new_dim_vec);
  return SUCCESS;
}

uint32_t GetC0(ge::DataType dtype) {
  if (vector_of_dtype_and_c0.empty()) {
    return SHAPE_NUMBER_16;
  } else {
    if (vector_of_dtype_and_c0.size() <= dtype) {
      FE_LOGW("vector_of_dtype_and_c0 size %zu <= dtype[%u]", vector_of_dtype_and_c0.size(), dtype);
      return SHAPE_NUMBER_16;
    }
    return vector_of_dtype_and_c0[dtype];
  }
}

int64_t ShapeTransferAccordingToFormat::GetAsisEnlargeValue(const int64_t& cin, const int64_t& cout, const int64_t& c0,
                                                            const int64_t& group) {
  if (cin == 0 || cout == 0) {
    return 0;
  }
  int64_t tmp = GetLeastCommonMultiple(GetLeastCommonMultiple(cin, c0) / cin, GetLeastCommonMultiple(cout, NI) / cout);
  return std::min(tmp, group);
}

void ShapeTransferAccordingToFormat::SetRNNAttr(const ShapeAndFormat &shape_and_format_info,
                                                std::vector<int64_t> &axis_value) {
  if (shape_and_format_info.new_format != ge::FORMAT_FRACTAL_ZN_RNN &&
      shape_and_format_info.new_format != ge::FORMAT_ND_RNN_BIAS) {
    return;
  }
  if (axis_value.size() < AXIS_BOTTOM) {
    return;
  }
  axis_value[AXIS_INPUT_SIZE] = shape_and_format_info.extra_attr.input_size;
  axis_value[AXIS_HIDEEN_SIZE] = shape_and_format_info.extra_attr.hidden_size;
  axis_value[AXIS_STATE_SIZE] = shape_and_format_info.extra_attr.state_size;
}

Status ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(ShapeAndFormat &shape_and_format_info, int64_t *c) {
  /* The default new shape is old shape */
  shape_and_format_info.new_shape = shape_and_format_info.old_shape;
  std::vector<int64_t> unknown_rank_shape = {-2};
  if (shape_and_format_info.old_shape.GetDims() == unknown_rank_shape) {
    FE_LOGD("Do not need to do shape transformation for unknown rank case.");
    return SUCCESS;
  }

  if (shape_and_format_info.old_format >= ge::FORMAT_END ||
      shape_and_format_info.new_format >= ge::FORMAT_END) {
    REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][GetShape] Old format %u or new format %u is invalid!",
                    shape_and_format_info.old_format, shape_and_format_info.new_format);
    return FAILED;
  }

  if ((shape_and_format_info.current_data_type == ge::DT_UNDEFINED)
      || (shape_and_format_info.current_data_type >= ge::DT_MAX)) {
    REPORT_FE_ERROR("[GraphOptJdgInst][ShapeTrans][GetShape] currentDataType %u is invalid!",
                    shape_and_format_info.current_data_type);
    return FAILED;
  }
  if (!AxisUtil::HasAxisValueFunc(shape_and_format_info.old_format)) {
    return SUCCESS;
  }

  auto iter_get_new_shape_func = get_new_shape_func_map.find(shape_and_format_info.new_format);
  if (iter_get_new_shape_func == get_new_shape_func_map.end()) {
    FE_LOGW("Can not get new shape of new format %u!", shape_and_format_info.new_format);
    return SUCCESS;
  }
  FE_LOGD("Original format %s, new format %s", ConstFormatToStr(shape_and_format_info.old_format).c_str(),
          ConstFormatToStr(shape_and_format_info.new_format).c_str());

  std::vector<int64_t> axis_value;
  for (uint32_t i = 0; i < AXIS_BOTTOM; i++) {
    axis_value.push_back(1);
  }
  if (shape_and_format_info.group_count > GROUPS_DEFAULT_VALUE) {
    axis_value[AXIS_G] = shape_and_format_info.group_count;
  }

  std::vector<int64_t> nd_value;
  uint32_t c0 = GetC0(shape_and_format_info.current_data_type);

  // The value of C0 should be 4 while format is 5HD-4 or FRAZ-4
  if (shape_and_format_info.new_format == ge::FORMAT_NC1HWC0_C04) {
    c0 = SHAPE_DIM_VALUE_C04;
  }

  Status status = AxisUtil::GetAxisValueByOriginFormat(shape_and_format_info.old_format,
                                                       shape_and_format_info.old_shape.GetDims(), c0,
                                                       axis_value, nd_value);
  if (status != SUCCESS && shape_and_format_info.new_format != ge::FORMAT_FRACTAL_NZ) {
    return SUCCESS;
  }

  SetRNNAttr(shape_and_format_info, axis_value);
  GetNewShapeByAxisValueAndFormatPtr get_new_shape_func = nullptr;
  FE_MAKE_SHARED(get_new_shape_func = iter_get_new_shape_func->second, return FAILED);
  FE_CHECK_NOTNULL(get_new_shape_func);
  (*get_new_shape_func)(shape_and_format_info.new_shape, shape_and_format_info.op_impl_type, axis_value, nd_value);
  if (c != nullptr) {
    *c = axis_value[AXIS_C];
  }
  return SUCCESS;
}
};  // namespace fe