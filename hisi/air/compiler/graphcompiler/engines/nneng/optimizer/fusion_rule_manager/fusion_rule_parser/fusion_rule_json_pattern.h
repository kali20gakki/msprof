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

#ifndef FUSION_ENGINE_OPTIMIZER_FUSION_RULE_MANAGER_FUSION_RULE_PARSER_FUSION_RULE_JSON_PATTERN_H_
#define FUSION_ENGINE_OPTIMIZER_FUSION_RULE_MANAGER_FUSION_RULE_PARSER_FUSION_RULE_JSON_PATTERN_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "fusion_rule_manager/fusion_rule_parser/attr_assignment_expression.h"

namespace fe {

class FusionRuleJsonPattern;
class FusionRuleJsonGraph;
class FusionRuleJsonEdge;
class FusionRuleJsonNode;
class FusionRuleJsonOuter;
class FusionRuleJsonAnchor;

using FusionRuleJsonPatternPtr = std::shared_ptr<FusionRuleJsonPattern>;
using FusionRuleJsonGraphPtr = std::shared_ptr<FusionRuleJsonGraph>;
using FusionRuleJsonEdgePtr = std::shared_ptr<FusionRuleJsonEdge>;
using FusionRuleJsonNodePtr = std::shared_ptr<FusionRuleJsonNode>;
using FusionRuleJsonOuterPtr = std::shared_ptr<FusionRuleJsonOuter>;
using FusionRuleJsonAnchorPtr = std::shared_ptr<FusionRuleJsonAnchor>;
using AttrAssignmentExpressionPtr = std::shared_ptr<AttrAssignmentExpression>;

struct FusionRuleJsonStruct {
  std::vector<nlohmann::json> outer_inputs;
  std::vector<nlohmann::json> outer_outputs;
  nlohmann::json origin_json_graph;
  nlohmann::json fusion_json_graph;
};

/** @brief Json file parse methods, parse json type fusion rule to c++ type,
*        doing both json syntax check, and fusion rule format check */
class FusionRuleJsonPattern {
 public:
  FusionRuleJsonPattern()
      : RULE_NAME("RuleName"),
        OUTER_INPUTS("OuterInputs"),
        OUTER_OUTPUTS("OuterOutputs"),
        ORIGIN_GRAPH("OriginGraph"),
        FUSION_GRAPH("FusionGraph") {}

  ~FusionRuleJsonPattern() {}
  FusionRuleJsonPattern(const FusionRuleJsonPattern &) = delete;
  FusionRuleJsonPattern &operator=(const FusionRuleJsonPattern &) = delete;

  Status ParseToJsonPattern(const nlohmann::json &json_object);

  std::string GetName() const { return name_; }

  const std::vector<FusionRuleJsonOuterPtr> &GetInputInfos() const { return input_infos_; }

  const std::vector<FusionRuleJsonOuterPtr> &GetOutputInfos() const { return output_infos_; }

  FusionRuleJsonGraphPtr GetOriginGraph() { return origin_graph_; }

  FusionRuleJsonGraphPtr GetFusionGraph() { return fusion_graph_; }

 private:
  /*
   * @brief: get each subitem of fusion rule to FusionRuleJsonStruct
   *         pattern json format: {
   *                                  "RuleName":"rule_name",
   *                                  "OuterInputs":[{outer_format},...],
   *                                  "OuterOutputs":[{outer_format},...],
   *                                  "OriginGraph":{graph_format},
   *                                  "FusionGraph":{graph_format}
   *                              }
   */
  Status LoadJson(const nlohmann::json &json_object, FusionRuleJsonStruct &fusion_rule_json_struct);

  static Status ParseToOuterInput(const std::vector<nlohmann::json> &outer_inputs,
                                  std::vector<FusionRuleJsonOuterPtr> &input_infos);
  static Status ParseToOuterOutput(const std::vector<nlohmann::json> &outer_outputs,
                                   std::vector<FusionRuleJsonOuterPtr> &output_infos);
  static Status ParseToOriginGraph(const nlohmann::json &json_object, FusionRuleJsonGraphPtr &origin_graph);
  static Status ParseToFusionGraph(const nlohmann::json &json_object, FusionRuleJsonGraphPtr &fusion_graph);

  const std::string RULE_NAME;
  const std::string OUTER_INPUTS;
  const std::string OUTER_OUTPUTS;
  const std::string ORIGIN_GRAPH;
  const std::string FUSION_GRAPH;

  std::string name_;
  std::vector<FusionRuleJsonOuterPtr> input_infos_;
  std::vector<FusionRuleJsonOuterPtr> output_infos_;
  FusionRuleJsonGraphPtr origin_graph_;
  FusionRuleJsonGraphPtr fusion_graph_;
};

class FusionRuleJsonGraph {
 public:
  FusionRuleJsonGraph() : NODES("Nodes"), EDGES("Edges"), ATTRS("Attrs"), has_attrs_(false) {}

  ~FusionRuleJsonGraph() {}
  FusionRuleJsonGraph(const FusionRuleJsonGraph &) = delete;
  FusionRuleJsonGraph &operator=(const FusionRuleJsonGraph &) = delete;
  /*
   * @breif: graph json format: {
   *                                "Nodes":[{node_format},...],
   *                                "Edges":[{edge_format},...],
   *                                "Attrs":[{attr_format},...]
   *                            }
   */
  Status ParseToJsonGraph(const nlohmann::json &json_object);

  Status ParseAllAttrValue(const std::map<string, std::vector<string>> &node_map);

  /*
   * @brief: add all nodes in current graph to input parameter: node_map.
   */
  Status GatherNode(map<string, vector<string>> &node_map);

  bool HasAttrs() { return has_attrs_; }

  const std::vector<FusionRuleJsonNodePtr> &GetNodes() const { return nodes_; }

  const std::vector<FusionRuleJsonEdgePtr> &GetEdges() const { return edges_; }

  const std::vector<AttrAssignmentExpressionPtr> &GetAttrAssigns() const { return attr_assigns_; }

 private:
  const std::string NODES;
  const std::string EDGES;
  const std::string ATTRS;

  bool has_attrs_;

  std::vector<FusionRuleJsonNodePtr> nodes_;
  std::vector<FusionRuleJsonEdgePtr> edges_;
  std::vector<AttrAssignmentExpressionPtr> attr_assigns_;
};

class FusionRuleJsonEdge {
 public:
  FusionRuleJsonEdge() : SRC("src"), DST("dst") {}

  ~FusionRuleJsonEdge() {}
  FusionRuleJsonEdge(const FusionRuleJsonEdge &) = delete;
  FusionRuleJsonEdge &operator=(const FusionRuleJsonEdge &) = delete;
  /*
   * @breif: edge json format: {
   *                               "src":{anchor_format},
   *                               "dst":{anchor_format}
   *                           }
   */
  Status ParseToJsonEdge(const nlohmann::json &json_object);

  Status ParseJson(const nlohmann::json &json_object);

  FusionRuleJsonAnchorPtr GetSrc() { return src_; }

  FusionRuleJsonAnchorPtr GetDst() { return dst_; }

 private:
  const std::string SRC;
  const std::string DST;

  FusionRuleJsonAnchorPtr src_;
  FusionRuleJsonAnchorPtr dst_;
};

class FusionRuleJsonNode {
 public:
  FusionRuleJsonNode() : NAME("name"), TYPE("type") {}
  ~FusionRuleJsonNode() {}
  FusionRuleJsonNode(const FusionRuleJsonNode &) = delete;
  FusionRuleJsonNode &operator=(const FusionRuleJsonNode &) = delete;
  /*
   * @breif: node json format: {
   *                               "name":"node_name",
   *                               "type":["type1"],
   *                           }
   */
  Status ParseToJsonNode(const nlohmann::json &json_object);

  Status ParseJson(const nlohmann::json &json_object);

  std::string GetName() { return name_; }

  const std::vector<std::string> &GetTypes() const { return types_; }

 private:
  const std::string NAME;
  const std::string TYPE;

  std::string name_;
  std::vector<std::string> types_;
};

class FusionRuleJsonOuter {
 public:
  FusionRuleJsonOuter() : NAME("name"), SRC("src"), has_src_(true), src_index_(-1) {}
  ~FusionRuleJsonOuter() {}
  FusionRuleJsonOuter(const FusionRuleJsonOuter &) = delete;
  FusionRuleJsonOuter &operator=(const FusionRuleJsonOuter &) = delete;
  /*
   * @brief: outer json format: {
   *                                "name":"outer_name",
   *                                "src":"node_name:index"
   *                            }
   */
  Status ParseToJsonOuter(const nlohmann::json &json_object);

  bool HasSrc() { return has_src_; }

  int GetSrcIndex() { return src_index_; }

  std::string GetSrcNode() { return src_node_; }

  std::string GetName() { return name_; }

 private:
  const std::string NAME;
  const std::string SRC;

  bool has_src_;
  int src_index_;
  std::string src_node_;
  std::string name_;
};

class FusionRuleJsonAnchor {
 public:
  FusionRuleJsonAnchor() : has_index_(true), src_index_(-1) {}

  ~FusionRuleJsonAnchor() {}
  FusionRuleJsonAnchor(const FusionRuleJsonAnchor &) = delete;
  FusionRuleJsonAnchor &operator=(const FusionRuleJsonAnchor &) = delete;
  /*
   * @brief: anchor json format: "node_name:index"/ "outer_name"
   */
  Status ParseToJsonAnchor(const nlohmann::json &json_object);

  bool HasIndex() { return has_index_; }

  int GetSrcIndex() { return src_index_; }

  std::string GetSrcNode() { return src_node_; }

  std::string GetName() { return name_; }

 private:
  bool has_index_;

  int src_index_;
  std::string src_node_;
  std::string name_;
};

}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_FUSION_RULE_MANAGER_FUSION_RULE_PARSER_FUSION_RULE_JSON_PATTERN_H_
