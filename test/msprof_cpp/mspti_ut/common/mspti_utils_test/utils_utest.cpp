/**
* @file callback_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/

#include "gtest/gtest.h"

#include <linux/limits.h>
#include "common/utils.h"

namespace {
class UtilsUtest : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_F(UtilsUtest, RealPathTest)
{
    std::string path = "./test";
    auto realPath = Mspti::Common::Utils::RealPath(path);
    char buf[PATH_MAX];
    getcwd(buf, PATH_MAX);
    std::string targetPath = std::string(buf) + "/test";
    EXPECT_STREQ(targetPath.c_str(), realPath.c_str());
}
}
