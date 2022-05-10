#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <iostream>
#include "errno/error_code.h"
#include "adx_prof_api.h"

using namespace Analysis::Dvvp::Adx;

class COMMON_ADX_PROF_API_STEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(COMMON_ADX_PROF_API_STEST, AdxIdeCreatePacket) {
    GlobalMockObject::verify();

    IdeBuffT outPut;
    int outLen = 0;
    EXPECT_EQ(IDE_DAEMON_ERROR, AdxIdeCreatePacket(NULL, 0, outPut, outLen));
}

TEST_F(COMMON_ADX_PROF_API_STEST, AdxIdeFreePacket) {
    GlobalMockObject::verify();
    IdeBuffT outPut = nullptr;
    AdxIdeFreePacket(outPut);
}

TEST_F(COMMON_ADX_PROF_API_STEST, AdxIdeSockReadData) {
    GlobalMockObject::verify();
    int outLen = 0;
    EXPECT_EQ(IDE_DAEMON_ERROR, AdxIdeSockReadData(NULL, NULL, outLen));
}

TEST_F(COMMON_ADX_PROF_API_STEST, AdxIdeSockWriteData) {
    GlobalMockObject::verify();
    IdeBuffT outPut;
    int outLen = 0;
    EXPECT_EQ(IDE_DAEMON_ERROR, AdxIdeSockWriteData(NULL, outPut, outLen));
}

TEST_F(COMMON_ADX_PROF_API_STEST, AdxIdeSockDupCreate) {
    GlobalMockObject::verify();
    EXPECT_EQ(nullptr, AdxIdeSockDupCreate(NULL));
}

TEST_F(COMMON_ADX_PROF_API_STEST, AdxIdeSockDestroy) {
    GlobalMockObject::verify();
    AdxIdeSockDestroy(NULL);
}

TEST_F(COMMON_ADX_PROF_API_STEST, AdxIdeSockDupDestroy) {
    GlobalMockObject::verify();
    AdxIdeSockDupDestroy(NULL);
}

TEST_F(COMMON_ADX_PROF_API_STEST, AdxGetAdxWorkPath) {
    GlobalMockObject::verify();
    EXPECT_STREQ("./", AdxGetAdxWorkPath().c_str());
}