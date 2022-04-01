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

#include "plugin/engine/dnnengines.h"

#include <string>

namespace ge {
AICoreDNNEngine::AICoreDNNEngine(const std::string &engine_name) {
  engine_attribute_.engine_name = engine_name;
  engine_attribute_.compute_cost = PriorityEnum::COST_0;
  engine_attribute_.runtime_type = DEVICE;
  engine_attribute_.engine_input_format = FORMAT_RESERVED;
  engine_attribute_.engine_output_format = FORMAT_RESERVED;
}

VectorCoreDNNEngine::VectorCoreDNNEngine(const std::string &engine_name) {
  engine_attribute_.engine_name = engine_name;
  engine_attribute_.compute_cost = PriorityEnum::COST_1;
  engine_attribute_.runtime_type = DEVICE;
  engine_attribute_.engine_input_format = FORMAT_RESERVED;
  engine_attribute_.engine_output_format = FORMAT_RESERVED;
}

AICpuDNNEngine::AICpuDNNEngine(const std::string &engine_name) {
  engine_attribute_.engine_name = engine_name;
  engine_attribute_.compute_cost = PriorityEnum::COST_2;
  engine_attribute_.runtime_type = DEVICE;
  engine_attribute_.engine_input_format = FORMAT_RESERVED;
  engine_attribute_.engine_output_format = FORMAT_RESERVED;
}

AICpuTFDNNEngine::AICpuTFDNNEngine(const std::string &engine_name) {
  engine_attribute_.engine_name = engine_name;
  engine_attribute_.compute_cost = PriorityEnum::COST_3;
  engine_attribute_.runtime_type = DEVICE;
  engine_attribute_.engine_input_format = FORMAT_RESERVED;
  engine_attribute_.engine_output_format = FORMAT_RESERVED;
}

GeLocalDNNEngine::GeLocalDNNEngine(const std::string &engine_name) {
  engine_attribute_.engine_name = engine_name;
  engine_attribute_.engine_input_format = FORMAT_RESERVED;
  engine_attribute_.engine_output_format = FORMAT_RESERVED;
}

HostCpuDNNEngine::HostCpuDNNEngine(const std::string &engine_name) {
  engine_attribute_.engine_name = engine_name;
  engine_attribute_.compute_cost = PriorityEnum::COST_10;
  engine_attribute_.runtime_type = HOST;
  engine_attribute_.engine_input_format = FORMAT_RESERVED;
  engine_attribute_.engine_output_format = FORMAT_RESERVED;
}

RtsDNNEngine::RtsDNNEngine(const std::string &engine_name) {
  engine_attribute_.engine_name = engine_name;
  engine_attribute_.engine_input_format = FORMAT_RESERVED;
  engine_attribute_.engine_output_format = FORMAT_RESERVED;
}

HcclDNNEngine::HcclDNNEngine(const std::string &engine_name) {
  engine_attribute_.engine_name = engine_name;
  engine_attribute_.engine_input_format = FORMAT_RESERVED;
  engine_attribute_.engine_output_format = FORMAT_RESERVED;
}

FftsPlusDNNEngine::FftsPlusDNNEngine(const std::string &engine_name) {
  engine_attribute_.engine_name = engine_name;
  engine_attribute_.engine_input_format = FORMAT_RESERVED;
  engine_attribute_.engine_output_format = FORMAT_RESERVED;
}

DSADNNEngine::DSADNNEngine(const std::string &engine_name) {
  engine_attribute_.engine_name = engine_name;
  engine_attribute_.compute_cost = PriorityEnum::COST_1;
  engine_attribute_.runtime_type = DEVICE;
  engine_attribute_.engine_input_format = FORMAT_RESERVED;
  engine_attribute_.engine_output_format = FORMAT_RESERVED;
}
}  // namespace ge
