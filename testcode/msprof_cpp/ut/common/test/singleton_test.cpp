#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "singleton/singleton.h"

using namespace analysis::dvvp::common::singleton;

class SingletonClass : public Singleton<SingletonClass> {
};

class COMMON_SINGLETON_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

TEST_F(COMMON_SINGLETON_TEST, instance) {
    GlobalMockObject::verify();

    EXPECT_NE((SingletonClass *)NULL, SingletonClass::instance());
    EXPECT_NE((SingletonClass *)NULL, SingletonClass::instance());
}