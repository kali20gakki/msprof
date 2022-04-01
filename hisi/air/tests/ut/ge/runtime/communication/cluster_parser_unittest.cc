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
#include <cstdlib>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "deploy/hcom/cluster/cluster_manager.h"
#include "deploy/hcom/cluster/cluster_parser.h"

namespace ge {

enum ClusterJsonTestCase {
  kRightJsonFormat = 0,
  kBadJsonFormat,
  kWithRepeatedIP,
  kWithUnknownMember,
  kWithoutWorker,
  kWithoutChief,
  kWithoutIP,
  kJsonTestCaseMax = kWithoutIP + 1
};

static const std::string kClusterJsonTestEnv[kJsonTestCaseMax] = {
    [kRightJsonFormat] = "{\"chief\":\"10.174.28.82:34961\",\"worker\":[\"1.2.1.1:1\",\"1.1.1.1:1\",\"1.1.2.1:1\"]}",
    [kBadJsonFormat] = "a",
    [kWithRepeatedIP] = "{\"chief\":\"10.174.28.82:34961\",\"worker\":[\"1.2.1.1:1\",\"1.1.2.1:1\",\"1.1.2.1:1\"]}",
    [kWithUnknownMember] = "{\"chief\":\"10.174.28.82:34961\",\"worker\":[\"1.2.1.1:1\",\"1.1.1.1:1\",\"1.1.3.1:1\"]}",
    [kWithoutWorker] = "{\"chief\":\"10.174.28.82:34961\"}",
    [kWithoutChief] = "{\"worker\":[\"1.2.1.1:1\",\"1.1.1.1:1\",\"1.1.2.1:1\"]}",
    [kWithoutIP] = "{\"chief\":\"\",\"worker\":[\"1.2.1.1:1\",\"1.1.1.1:1\",\"1.1.2.1:1\"]}"};

class UtClusterParser : public testing::Test {
 public:
  UtClusterParser() {}

 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(UtClusterParser, run_cluster_parser_right_format) {
  // setup
  setenv("HELP_CLUSTER", kClusterJsonTestEnv[kRightJsonFormat].c_str(), 1);

  ClusterParser parser;
  ClusterMemberInfo member_info;
  member_info.local_addr = "10.174.28.82:34961";

  // test
  auto ret = parser.MemberParser(member_info);
  ASSERT_EQ(ret, SUCCESS);

  // check
  ASSERT_EQ(member_info.member_type, ClusterMemberType::kChief);
  ASSERT_STREQ(member_info.local_addr.c_str(), "10.174.28.82:34961");
  ASSERT_STREQ(member_info.chief_addr.c_str(), "10.174.28.82:34961");
  ASSERT_EQ(member_info.cluster_member_num, 4);

  auto it = member_info.members.find("1.2.1.1:1");
  ASSERT_TRUE(it != member_info.members.end());
  ASSERT_EQ(it->second, ClusterMemberStatus::kInit);

  it = member_info.members.find("1.1.1.1:1");
  ASSERT_TRUE(it != member_info.members.end());
  ASSERT_EQ(it->second, ClusterMemberStatus::kInit);

  it = member_info.members.find("1.1.2.1:1");
  ASSERT_TRUE(it != member_info.members.end());
  ASSERT_EQ(it->second, ClusterMemberStatus::kInit);

  it = member_info.members.find("1.1.2.3:1");
  ASSERT_TRUE(it == member_info.members.end());
  // TearDown
  unsetenv("HELP_CLUSTER");
}

TEST_F(UtClusterParser, run_cluster_parser_repeated_ip) {
  // setup
  setenv("HELP_CLUSTER", kClusterJsonTestEnv[kWithRepeatedIP].c_str(), 1);

  ClusterParser parser;
  ClusterMemberInfo member_info;
  member_info.local_addr = "10.174.28.82:34961";

  // test
  auto ret = parser.MemberParser(member_info);
  ASSERT_NE(ret, SUCCESS);

  // TearDown
  unsetenv("HELP_CLUSTER");
}

TEST_F(UtClusterParser, run_cluster_parser_unknown_member) {
  // setup
  setenv("HELP_CLUSTER", kClusterJsonTestEnv[kWithUnknownMember].c_str(), 1);

  ClusterParser parser;
  ClusterMemberInfo member_info;
  member_info.local_addr = "1.1.2.1";

  // test
  auto ret = parser.MemberParser(member_info);
  ASSERT_EQ(ret, SUCCESS);

  // TearDown
  unsetenv("HELP_CLUSTER");
}

TEST_F(UtClusterParser, run_cluster_parser_with_out_worker) {
  // setup
  setenv("HELP_CLUSTER", kClusterJsonTestEnv[kWithoutWorker].c_str(), 1);

  ClusterParser parser;
  ClusterMemberInfo member_info;
  member_info.local_addr = "10.174.28.82:34961";

  // test
  auto ret = parser.MemberParser(member_info);
  ASSERT_EQ(ret, SUCCESS);

  // TearDown
  unsetenv("HELP_CLUSTER");
}

TEST_F(UtClusterParser, run_cluster_parser_with_out_chief) {
  // setup
  setenv("HELP_CLUSTER", kClusterJsonTestEnv[kWithoutChief].c_str(), 1);

  ClusterParser parser;
  ClusterMemberInfo member_info;
  member_info.local_addr = "1.1.2.1";

  // test
  auto ret = parser.MemberParser(member_info);
  ASSERT_EQ(ret, FAILED);

  // TearDown
  unsetenv("HELP_CLUSTER");
}

TEST_F(UtClusterParser, run_cluster_parser_with_out_ip) {
  // setup
  setenv("HELP_CLUSTER", kClusterJsonTestEnv[kWithoutIP].c_str(), 1);

  ClusterParser parser;
  ClusterMemberInfo member_info;
  member_info.local_addr = "1.1.2.1";

  // test
  auto ret = parser.MemberParser(member_info);
  ASSERT_NE(ret, SUCCESS);

  // TearDown
  unsetenv("HELP_CLUSTER");
}

TEST_F(UtClusterParser, run_cluster_parser_with_out_env) {
  // setup

  ClusterParser parser;
  ClusterMemberInfo member_info;
  member_info.local_addr = "1.1.2.1";

  // test
  auto ret = parser.MemberParser(member_info);
  ASSERT_NE(ret, SUCCESS);
}

TEST_F(UtClusterParser, run_cluster_parser_bad_formate) {
  // setup
  setenv("HELP_CLUSTER", kClusterJsonTestEnv[kBadJsonFormat].c_str(), 1);

  ClusterParser parser;
  ClusterMemberInfo member_info;
  member_info.local_addr = "1.1.2.1";

  // test
  auto ret = parser.MemberParser(member_info);
  ASSERT_NE(ret, SUCCESS);

  // TearDown
  unsetenv("HELP_CLUSTER");
}

}  // namespace ge
