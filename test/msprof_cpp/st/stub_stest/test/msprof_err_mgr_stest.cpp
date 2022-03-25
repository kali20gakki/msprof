#include "mockcpp/mockcpp.hpp"
#include "gtest/gtest.h"
#include <iostream>
#include <fstream>

#include "msprof_error_manager.h"
using namespace Analysis::Dvvp::MsprofErrMgr;
class ERR_MGR_STEST: public testing::Test {
protected:
    virtual void SetUp() {

    }
    virtual void TearDown() {
    }
};


TEST_F(ERR_MGR_STEST, GetErrorManagerContext)
{
    GlobalMockObject::verify();
    error_message::Context context = {0UL, "", "", ""};
    MOCKER_CPP(&ErrorManager::GetErrorManagerContext)
        .stubs()
        .will(returnValue(context));
    auto err_message = MsprofErrorManager::instance()->GetErrorManagerContext();
    EXPECT_EQ(0UL, err_message.work_stream_id);
}