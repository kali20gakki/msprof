#include "mockcpp/mockcpp.hpp"
#include "gtest/gtest.h"
#include "errno/error_code.h"
#include "input_parser.h"
#include "config/config_manager.h"
#include "mmpa_api.h"

using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Msprof;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;

class INPUT_PARSER_UTEST : public testing::Test {
protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST_F(INPUT_PARSER_UTEST, MsoprofTaskExecMsopprof)
{
    GlobalMockObject::verify();
    MsoprofTask msopprof_task = MsoprofTask();
    CONST_CHAR_PTR path = "cmd";
    std::vector<std::string> argsVec;
    MOCKER_CPP(&analysis::dvvp::common::utils::Utils::ExecCmd)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, msopprof_task.ExecMsopprof(path, argsVec));
}
