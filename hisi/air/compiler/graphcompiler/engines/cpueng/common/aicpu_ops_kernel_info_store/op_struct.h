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
#ifndef AICPU_OP_STRUCT_H_
#define AICPU_OP_STRUCT_H_

#include <map>
#include <string>
#include <vector>
#include "cce/fwk_adpt_struct.h"

namespace aicpu {

struct OpFullInfo {
  std::string engine;        // which engine
  std::string opKernelLib;   // which opsKernelStore
  int computeCost;           // compute cost
  bool flagPartial;          // whether to support is related to shape
  bool flagAsync;            // Whether to support asynchronous
  std::string kernelSo;      // kernel so
  std::string functionName;  // function name
  bool userDefined;          // user defined
  std::map<std::string, std::string> inOutFormat;  // input output format
  std::string opsFlag;  // opsFlag[0] indicates constant folding
  int shapeType;
  std::map<std::string, std::string> inOutDataType;  // input output DataType
  std::map<std::string, std::string> inOutRealName;  // input output name
  bool formatAgnostic;                               // set format agnostic
  int workspaceSize;                                 // workspace size
  std::map<std::string, std::string> castSrcType;
  std::map<std::string, std::string> castDstType;
  FWKAdapter::FWKExtTopicType topicType; // support: DEVICE_ONLY/DEVICE_FIRST/HOST_ONLY/HOST_FIRST
  std::string resource;
  bool noTiling;                         // whether to support no tiling
  std::string slicePattern; // STR_SLICE_PATTERN_VEC
  bool flagSupportBlockDim;
  int blockDimByIndex;
};

struct OpInfoDesc {
  std::string opName;  // op name
  OpFullInfo opInfo;   // engine information that the op
};

struct OpInfoDescs {
  std::vector<OpInfoDesc> opInfos;  // op info
  std::string libName;              // kernel lib name
};
}  // namespace aicpu

#endif