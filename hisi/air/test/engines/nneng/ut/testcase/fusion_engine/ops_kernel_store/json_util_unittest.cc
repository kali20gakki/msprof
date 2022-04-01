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

#include "common/util/json_util.h"

#include <gtest/gtest.h>
#include <fcntl.h>
#include "sys/stat.h"

using namespace std;
using namespace testing;
using namespace fe;
using namespace nlohmann;

class JsonUtilTest : public testing::Test{
  protected:
    static void SetUpTestCase() {
        std::cout << "JsonUtilTest SetUP" << std::endl;
    }
    static void TearDownTestCase() {
        std::cout << "JsonUtilTest SetUP" << std::endl;
    }
    // Some expensive resource shared by all tests.
    virtual void SetUp(){
        std::cout << "a test SetUP" << std::endl;
    }
    virtual void TearDown(){
        std::cout << "a test SetUP" << std::endl;

    }
};

TEST_F(JsonUtilTest, real_path_fail)
{
    std::string filename = "tbe_custom_opinfo.json";
    string ret = RealPath(filename);
    EXPECT_EQ(0, ret.size());
}

TEST_F(JsonUtilTest, real_path_succ)
{
    std::string filename = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/cce_custom_opinfo/cce_custom_opinfo.json";
    string ret = RealPath(filename);
    EXPECT_NE(0, ret.size());
}

TEST_F(JsonUtilTest, read_json_file_succ)
{
    std::string filename = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/cce_custom_opinfo/cce_custom_opinfo.json";
    json j;
    Status ret = ReadJsonFile(filename, j);
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(JsonUtilTest, read_json_file_fail_)
{
    std::string filename = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/opinfo_not_exist.json";
    json j;
    Status ret = ReadJsonFile(filename, j);
    EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(JsonUtilTest, read_json_file_lock_succ)
{
    std::string filename = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/cce_custom_opinfo/cce_custom_opinfo.json";
    json j;
    Status ret = ReadJsonFileByLock(filename, j);
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(JsonUtilTest, read_json_file_lock_nonexist_fail)
{
    std::string filename = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/opinfo_not_exist.json";
    json j;
    Status ret = ReadJsonFileByLock(filename, j);
    EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(JsonUtilTest, read_json_file_lock_fail)
{
    std::string filename = "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/cce_custom_opinfo/cce_custom_opinfo.json";

    // manually lock first
    FILE *fp = fopen(filename.c_str(), "r");
    if (fp == nullptr) {
        EXPECT_EQ(true, false);
    }
    if (FcntlLockFile(filename, fileno(fp), -88, 0) != fe::FAILED) {
        EXPECT_EQ(true, false);
    } else {
        (void)FcntlLockFile(filename, fileno(fp), F_UNLCK, 0);
    }
    fclose(fp);
}
