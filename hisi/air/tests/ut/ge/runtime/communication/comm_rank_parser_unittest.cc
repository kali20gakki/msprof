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
#include "deploy/hcom/rank_parser.h"

namespace ge {

enum ClusterJsonTestCase {
  kRightRankFormat = 0,
  kBadJsonFormat = 1,
  kJasonTestCaseMax = kBadJsonFormat + 1,
};

static const std::string kHcclJsonTestInfo[kJasonTestCaseMax] = {
    [kRightRankFormat] =
        "{"
        "	\"rankSize\": 4,"
        "	\"rankTable\": [{"
        "			\"rank_id\": 0,"
        "			\"model_instance_id\": 0"
        "		},"
        "		{"
        "			\"rank_id\": 1,"
        "			\"model_instance_id\": 1"
        "		},"
        "		{"
        "			\"rank_id\": 2,"
        "			\"model_instance_id\": 2"
        "		},"
        "		{"
        "			\"rank_id\": 3,"
        "			\"model_instance_id\": 3"
        "		},"
        "		{"
        "			\"rank_id\": 4,"
        "			\"model_instance_id\": 4"
        "		}"
        "	],"
        "	\"subGroups\": [{"
        "			\"group_id\": \"group1\","
        "			\"group_rank_list\": [0,2]"
        "		},"
        "		{"
        "			\"group_id\": \"group2\","
        "			\"group_rank_list\": [1,3]"
        "		}"
        "	]"
        "}",
    [kBadJsonFormat] = "a",
};

static const char *kDepolyJsonTestInfo[kJasonTestCaseMax] = {
    [kRightRankFormat] =
        "[{"
        "		\"model_instance_id\": 0,"
        "		\"device_id\": 0,"
        "		\"model_id\": \"model1\""
        "	},"
        " {"
        "		\"model_instance_id\": 1,"
        "		\"device_id\": 0,"
        "		\"model_id\": \"model1\""
        "	},"
        "	{"
        "		\"model_instance_id\": 2,"
        "		\"device_id\": 0,"
        "		\"model_id\": \"model1\""
        "	},"
        "	{"
        "		\"model_instance_id\": 3,"
        "		\"device_id\": 1,"
        "		\"model_id\": \"model2\""
        "	},"
        "	{"
        "		\"model_instance_id\": 4,"
        "		\"device_id\": 2,"
        "		\"model_id\": \"model3\""
        "	}"
        "]",
    [kBadJsonFormat] = "a",
};

class UtRankParser : public testing::Test {
 public:
  UtRankParser() {}

 protected:
  void SetUp() override {
    AutoArrangeRankParser::GetInstance().Reset();
  }
  void TearDown() override {
    AutoArrangeRankParser::GetInstance().Reset();
  }
};

TEST_F(UtRankParser, run_rank_parser_right_format) {
  auto &rank_parser = RankParser::GetInstance();
  auto ret = rank_parser.Init(kHcclJsonTestInfo[kRightRankFormat], kDepolyJsonTestInfo[kRightRankFormat]);
  ASSERT_TRUE(ret == SUCCESS);

  auto v0 = rank_parser.GetRanks(0);
  std::vector<uint32_t> v_r0 = {0, 1, 2};

  for (auto f : v0) {
    std::cout << f;
  }

  EXPECT_EQ(v0, v_r0);

  auto v1 = rank_parser.GetRanks(1);
  std::vector<uint32_t> v_r1 = {3};
  EXPECT_EQ(v1, v_r1);

  auto v2 = rank_parser.GetRanks(2);
  std::vector<uint32_t> v_r2 = {4};
  EXPECT_EQ(v2, v_r2);

  auto m = rank_parser.GetSubGroups();
  for (auto f : m) {
    if (f.first == "group1") {
      std::vector<uint32_t> v = {0, 2};
      EXPECT_EQ(f.second, v);
    }
    if (f.first == "group2") {
      std::vector<uint32_t> v = {1, 3};
      EXPECT_EQ(f.second, v);
    }
  }

  std::vector<uint32_t> v3;
  for (int32_t i = 0; i < 5; i++) {
    uint32_t device_id;
    rank_parser.GetDeviceFromeRank(i, &device_id);
    v3.emplace_back(device_id);
  }
  std::vector<uint32_t> v_r3 = {0, 0, 0, 1, 2};
  EXPECT_EQ(v3, v_r3);

  std::vector<uint32_t> v4;
  for (int32_t i = 0; i < 5; i++) {
    uint32_t rank_id;
    rank_parser.GetRankFromSubModel(i, &rank_id);
    v4.emplace_back(rank_id);
  }
  std::vector<uint32_t> v_r4 = {0, 1, 2, 3, 4};
  EXPECT_EQ(v4, v_r4);
}

TEST_F(UtRankParser, run_rank_parser_bad_format) {
  auto &rank_parser = RankParser::GetInstance();
  auto ret = rank_parser.Init(kHcclJsonTestInfo[kBadJsonFormat], kDepolyJsonTestInfo[kBadJsonFormat]);
  ASSERT_TRUE(ret == FAILED);

  ret = rank_parser.Init(kHcclJsonTestInfo[kRightRankFormat], kDepolyJsonTestInfo[kBadJsonFormat]);
  ASSERT_TRUE(ret == FAILED);
}
TEST_F(UtRankParser, run_auto_rank_parser_rank_id_0) {
  setenv("RANK_ID", "0", 1);
  uint32_t type = AutoArrangeRankParser::GetInstance().GetAutoArrangeRankType();
  ASSERT_EQ(type, 0xaa);
  uint32_t rank_id = AutoArrangeRankParser::GetInstance().GetAutoArrangeRankId(type);
  ASSERT_EQ(rank_id, 0);
  for (int32_t i = 0; i < 10; i++) {
    rank_id = AutoArrangeRankParser::GetInstance().GetAutoArrangeRankId(0);
    ASSERT_EQ(rank_id, i + 1);
  }

  unsetenv("RANK_ID");
}

TEST_F(UtRankParser, run_auto_rank_parser_rank_id_1) {
  setenv("RANK_ID", "1", 1);
  uint32_t type = AutoArrangeRankParser::GetInstance().GetAutoArrangeRankType();
  ASSERT_EQ(type, 0);
  unsetenv("RANK_ID");
}

TEST_F(UtRankParser, run_auto_rank_parser_bad_rank_id) {
  setenv("RANK_ID", "18446744073709551616", 1);
  uint32_t type = AutoArrangeRankParser::GetInstance().GetAutoArrangeRankType();
  ASSERT_EQ(type, 0);
  unsetenv("RANK_ID");
}

TEST_F(UtRankParser, run_auto_rank_parser_without_rank_id) {
  uint32_t type = AutoArrangeRankParser::GetInstance().GetAutoArrangeRankType();
  ASSERT_EQ(type, 0);
}
}  // namespace ge
