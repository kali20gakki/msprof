/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#include "prof_api_test.h"
#include "acl/acl_prof.h"
#include "prof_api_common.h"

class AclApiTest : public testing::Test {
public:
    AclApiTest() = default;
    ~AclApiTest() = default;

protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_F(AclApiTest, aclprofInit)
{
    char testApi[] = "test_aclprofInit";
    DT_FUZZ_START(0, g_fuzzRunTime, testApi, 0)
    {
        printf("\r%d", fuzzSeed + fuzzi);
        char* resultPath = DT_SetGetString(&g_Element[0], 5, 5000, "/tmp");
        int len = DT_GET_MutatedValueLen(&g_Element[0]);
        aclprofInit(resultPath, strlen(resultPath));
    }
    DT_FUZZ_END()
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, aclprofInit(nullptr, 0));
    char validpath[] = "/tmp/acl_fuzztest_new";
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, aclprofInit(validpath, strlen(validpath) + 1));
}
