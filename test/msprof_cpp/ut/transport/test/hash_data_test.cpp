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
 * -------------------------------------------------------------------------*/
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "securec.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <locale>
#include <errno.h>
#include <algorithm>
#include <fstream>
//mac
#include <net/if.h>
#include <sys/prctl.h>
#define protected public
#define private public
#include "message/message.h"
#include "thread/thread_pool.h"
#include "config/config_manager.h"
#include "param_validation.h"
#include "transport/hash_data.h"
#include "uploader_mgr.h"

using namespace analysis::dvvp::common::thread;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::transport;


class COMMON_HASH_DATA_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
        GlobalMockObject::verify();
        HashData::instance()->Uninit();
    }
private:
};

TEST_F(COMMON_HASH_DATA_TEST, Init) {
    int32_t ret  = HashData::instance()->Init();
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret  = HashData::instance()->Init();
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    auto iter = HashData::instance()->hashMapMutex_.find("DATA_PREPROCESS");
    EXPECT_NE(HashData::instance()->hashMapMutex_.end(), iter);
    HashData::instance()->Uninit();
}

TEST_F(COMMON_HASH_DATA_TEST, Uninit) {
    int32_t ret  = HashData::instance()->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(COMMON_HASH_DATA_TEST, IsInit) {
    HashData::instance()->Init();
    EXPECT_EQ(true, HashData::instance()->IsInit());
    HashData::instance()->Uninit();
    EXPECT_EQ(false, HashData::instance()->IsInit());    
}

TEST_F(COMMON_HASH_DATA_TEST, GenHashId) {
    HashData::instance()->Init();

    EXPECT_EQ(0, HashData::instance()->GenHashId("xxx", nullptr, 0));

    const char *data = "ABCDEFGHIJK";
    uint64_t hashId = 4667050837169873462;
    EXPECT_EQ(hashId, HashData::instance()->GenHashId("DATA_PREPROCESS", data, strlen(data)));
    std::string dataStr = HashData::instance()->GetHashData("DATA_PREPROCESS", hashId);
    EXPECT_EQ(0, strcmp(data, dataStr.c_str()));

    hashId = HashData::instance()->GenHashId("DATA_PREPROCESS", data, strlen(data));
    EXPECT_EQ(4667050837169873462, hashId);

    HashData::instance()->Uninit();
}

TEST_F(COMMON_HASH_DATA_TEST, GetHashData) {
    HashData::instance()->Init();
    std::string dataStr = HashData::instance()->GetHashData("xxx", 0);
    EXPECT_EQ(0, dataStr.size());

    uint64_t hashId = 4667050837169873462;
    std::string dataStr2 = HashData::instance()->GetHashData("xxx", hashId);
    EXPECT_EQ(0, dataStr2.size());

    HashData::instance()->Uninit();
}

TEST_F(COMMON_HASH_DATA_TEST, SaveHashData) {
    GlobalMockObject::verify();
    MOCKER_CPP(&analysis::dvvp::transport::UploaderMgr::UploadData,
        int(analysis::dvvp::transport::UploaderMgr::*)
        (const std::string&, SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk>))
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    HashData::instance()->SaveHashData();
    HashData::instance()->Init();
    const char *data = "ABCDEFGHIJK";
    HashData::instance()->GenHashId("DATA_PREPROCESS", data, strlen(data));
    HashData::instance()->GenHashId("AclModule", data, strlen(data));
    HashData::instance()->SaveHashData();
    HashData::instance()->SaveHashData();
    HashData::instance()->SaveHashData();
    HashData::instance()->Uninit();
}

TEST_F(COMMON_HASH_DATA_TEST, GetHashInfo)
{
    GlobalMockObject::verify();
    EXPECT_EQ("", HashData::instance()->GetHashInfo(0));
    
    uint64_t hashId = HashData::instance()->GenHashId("Test");
    EXPECT_EQ("Test", HashData::instance()->GetHashInfo(hashId));
}
 
TEST_F(COMMON_HASH_DATA_TEST, GenHashId_NewDataStruct)
{
    GlobalMockObject::verify();
    uint64_t hashId1 = HashData::instance()->GenHashId("Test");
    uint64_t hashId2 = HashData::instance()->GenHashId("Test");
    EXPECT_EQ(hashId1, hashId2);
}

TEST_F(COMMON_HASH_DATA_TEST, SaveNewHashData)
{
    GlobalMockObject::verify();
    HashData::instance()->SaveNewHashData(true);
 
    HashData::instance()->Init();
    HashData::instance()->GenHashId("Test1");
    HashData::instance()->GenHashId("Test2");
    MOCKER_CPP(&analysis::dvvp::transport::UploaderMgr::UploadData,
        int(analysis::dvvp::transport::UploaderMgr::*)
        (const std::string&, SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk>))
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    HashData::instance()->SaveNewHashData(true);
    HashData::instance()->SaveNewHashData(true);
}