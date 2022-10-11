#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adx_prof_api.h"
#include "memory_utils.h"

using namespace Analysis::Dvvp::Adx;

class ADX_PROF_API_UTEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {

    }
};


TEST_F(ADX_PROF_API_UTEST, AdxIdeCreatePacket) {
    GlobalMockObject::verify();

    IdeBuffT outPut;
    int outLen = 0;
    EXPECT_EQ(IDE_DAEMON_ERROR, AdxIdeCreatePacket(NULL, 0, outPut, outLen));

    EXPECT_EQ(IDE_DAEMON_OK, AdxIdeCreatePacket("test", 0, outPut, outLen));
    AdxIdeFreePacket(outPut);
}

TEST_F(ADX_PROF_API_UTEST, AdxIdeFreePacket) {
    GlobalMockObject::verify();
    IdeBuffT outPut = nullptr;
    AdxIdeFreePacket(outPut);
}