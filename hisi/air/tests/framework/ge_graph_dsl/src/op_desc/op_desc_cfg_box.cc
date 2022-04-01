/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#include "ge_graph_dsl/op_desc/op_desc_cfg_box.h"
#include "easy_graph/infra/status.h"
#include "ge_graph_dsl/op_desc/op_desc_cfg_repo.h"
#include "external/graph/gnode.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_tensor.h"

using ::EG_NS::Status;
using ::GE_NS::OpDescCfg;

GE_NS_BEGIN

OpDescCfgBox::OpDescCfgBox(const OpType &opType) : OpDescCfg(opType) {
  auto opCfg = OpDescCfgRepo::GetInstance().FindBy(opType);
  if (opCfg != nullptr) {
    ::OpDescCfg &base = *this;
    base = (*opCfg);
  }
}

OpDescCfgBox &OpDescCfgBox::InCnt(int in_cnt) {
  this->in_cnt_ = in_cnt;
  return *this;
}

OpDescCfgBox &OpDescCfgBox::OutCnt(int out_cnt) {
  this->out_cnt_ = out_cnt;
  return *this;
}

OpDescCfgBox &OpDescCfgBox::ParentNodeIndex(int node_index) {
  this->Attr(ATTR_NAME_PARENT_NODE_INDEX, node_index);
  return *this;
}

OpDescCfgBox &OpDescCfgBox::Attr(const std::string &name, int32_t value) {
  this->Attr(name, (int64_t)value);
  return *this;
}

OpDescCfgBox &OpDescCfgBox::Attr(const std::string &name, uint32_t value) {
  this->Attr(name, (int64_t)value);
  return *this;
}

OpDescCfgBox &OpDescCfgBox::Attr(const std::string &name, const std::vector<int32_t> &value) {
  std::vector<int64_t> int64_list;
  for (const auto &val : value) {
    int64_list.emplace_back(static_cast<int64_t>(val));
  }
  return Attr(name, int64_list);
}

OpDescCfgBox &OpDescCfgBox::Attr(const std::string &name, const std::vector<uint32_t> &value) {
  std::vector<int64_t> int64_list;
  for (const auto &val : value) {
    int64_list.emplace_back(static_cast<int64_t>(val));
  }
  return Attr(name, int64_list);
}

OpDescCfgBox &OpDescCfgBox::Attr(const std::string &name, const std::vector<int64_t> &value) {
  ge::GeAttrValue attrvalue;
  attrvalue.SetValue(value);
  attrs_.emplace(std::make_pair(name, attrvalue));
  return *this;
}

OpDescCfgBox &OpDescCfgBox::Attr(const std::string &name, const char *value) {
  this->Attr(name, std::string(value));
  return *this;
}

OpDescCfgBox &OpDescCfgBox::Weight(GeTensorPtr &tensor_ptr) {
  this->Attr<GeTensor>(ATTR_NAME_WEIGHTS, *tensor_ptr);
  return *this;
}

OpDescCfgBox &OpDescCfgBox::TensorDesc(Format format, DataType data_type, std::vector<int64_t> shape) {
  default_tensor_.format_ = format;
  default_tensor_.data_type_ = data_type;
  default_tensor_.shape_ = shape;
  return *this;
}

void OpDescCfgBox::UpdateAttrs(OpDescPtr &op_desc) const {
  std::for_each(attrs_.begin(), attrs_.end(),
                [&op_desc](const auto &attr) { op_desc->SetAttr(attr.first, attr.second); });
}

OpDescPtr OpDescCfgBox::Build(const ::EG_NS::NodeId &id) const {
  auto opPtr = std::make_shared<OpDesc>(id, GetType());
  GeTensorDesc tensor_desc(ge::GeShape(default_tensor_.shape_), default_tensor_.format_, default_tensor_.data_type_);
  tensor_desc.SetOriginShape(ge::GeShape(default_tensor_.shape_));
  if (in_names_.empty()) {
    for (int i = 0; i < in_cnt_; i++) {
      opPtr->AddInputDesc(tensor_desc);
    }
  } else {
    for (const auto &name : in_names_) {
      opPtr->AddInputDesc(name, tensor_desc);
    }
  }

  if (out_names_.empty()) {
    for (int i = 0; i < out_cnt_; i++) {
      opPtr->AddOutputDesc(tensor_desc);
    }
  } else {
    for (const auto &name : out_names_) {
      opPtr->AddOutputDesc(name, tensor_desc);
    }
  }

  UpdateAttrs(opPtr);
  return opPtr;
}
OpDescCfgBox &OpDescCfgBox::InNames(std::initializer_list<std::string> names) {
  in_names_.insert(in_names_.end(), names.begin(), names.end());
  in_cnt_ = static_cast<int32_t>(in_names_.size());
  return *this;
}
OpDescCfgBox &OpDescCfgBox::OutNames(std::initializer_list<std::string> names) {
  out_names_.insert(out_names_.end(), names.begin(), names.end());
  out_cnt_ = static_cast<int32_t>(out_names_.size());
  return *this;
}

GE_NS_END
