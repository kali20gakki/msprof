/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
*/

#include <cstdlib>
#include <fstream>
#include <libgen.h>
#include <linux/limits.h>

#include "gtest/gtest.h"

#include "csrc/common/utils.h"
#include "csrc/common/function_loader.h"

namespace {
class FunctionLoaderUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        std::string soPath = "libruntime";
        std::string funcName = "rtFree";
        Mspti::Common::RegisterFunction(soPath, funcName);
    }
    virtual void TearDown() {}
};

TEST_F(FunctionLoaderUtest, CanonicalSoPathShouldGetEmptyPathWhenInputInvalidSoPath)
{
    std::string notSupPath = "libnotsupport";
    const std::string empty = "";
    auto loader = Mspti::Common::FunctionLoader(notSupPath);
    EXPECT_STREQ(empty.c_str(), loader.CanonicalSoPath().c_str());
}

TEST_F(FunctionLoaderUtest, CanonicalSoPathShouldGetOriginPathWhenEnvNotSet)
{
    putenv("ASCEND_HOME_PATH=");
    std::string profapiSo = "libprofapi";
    auto loader = Mspti::Common::FunctionLoader(profapiSo);
    EXPECT_STREQ("libprofapi.so", loader.CanonicalSoPath().c_str());
}

TEST_F(FunctionLoaderUtest, CanonicalSoPathShouldGetRealPathWhenEnvAndFileReadable)
{
    char pwdPath[PATH_MAX] = {0};
    readlink("/proc/self/exe", pwdPath, PATH_MAX);
    ASSERT_TRUE(pwdPath != nullptr);
    char* dirName = dirname(pwdPath);
    ASSERT_TRUE(dirName != nullptr);

    std::string profapiSo = "libprofapi";
    std::string env = "ASCEND_HOME_PATH=" + std::string(dirName);
    putenv(const_cast<char*>(env.data()));

    std::string stubSo = std::string(dirName) + "/lib64/" + profapiSo + ".so";
    auto loader = Mspti::Common::FunctionLoader(profapiSo);
    EXPECT_STREQ(stubSo.c_str(), loader.CanonicalSoPath().c_str());
}

TEST_F(FunctionLoaderUtest, FunctionLoaderGetFuncPtrRightWhenSet)
{
    char pwdPath[PATH_MAX] = {0};
    readlink("/proc/self/exe", pwdPath, PATH_MAX);
    ASSERT_TRUE(pwdPath != nullptr);
    char* dirName = dirname(pwdPath);
    ASSERT_TRUE(dirName != nullptr);

    std::string profapiSo = "libprofapi";
    std::string env = "ASCEND_HOME_PATH=" + std::string(dirName);
    putenv(const_cast<char*>(env.data()));

    std::string stubSo = std::string(dirName) + "/lib64/" + profapiSo + ".so";
    auto loader = Mspti::Common::FunctionLoader(profapiSo);
    loader.Set("ProfSetProfCommand");
    EXPECT_NE(loader.Get("ProfSetProfCommand"), nullptr);
}

TEST_F(FunctionLoaderUtest, FunctionLoaderGetNullptrWhenNotSet)
{
    char pwdPath[PATH_MAX] = {0};
    readlink("/proc/self/exe", pwdPath, PATH_MAX);
    ASSERT_TRUE(pwdPath != nullptr);
    char* dirName = dirname(pwdPath);
    ASSERT_TRUE(dirName != nullptr);

    std::string profapiSo = "libprofapi";
    std::string env = "ASCEND_HOME_PATH=" + std::string(dirName);
    putenv(const_cast<char*>(env.data()));

    std::string stubSo = std::string(dirName) + "/lib64/" + profapiSo + ".so";
    auto loader = Mspti::Common::FunctionLoader(profapiSo);
    EXPECT_EQ(loader.Get("ProfSetProfCommand"), nullptr);
}

TEST_F(FunctionLoaderUtest, FunctionLoaderGetNullptrWhenDlopenFail)
{
    std::string soPath = "libruntime";
    auto loader = Mspti::Common::FunctionLoader(soPath);
    loader.Set("ProfSetProfCommand");
    EXPECT_EQ(loader.Get("ProfSetProfCommand"), nullptr);
}

TEST_F(FunctionLoaderUtest, FunctionRegisterGetSuccess)
{
    std::string soPath = "libruntime";
    std::string funcName = "rtFree";
    auto instance = Mspti::Common::FunctionRegister::GetInstance();
    auto func = Mspti::Common::RegisterFunction(soPath, funcName);
    EXPECT_EQ(instance->Get(soPath, funcName), func);
}
}
