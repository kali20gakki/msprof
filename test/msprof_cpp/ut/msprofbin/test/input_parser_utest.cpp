#include "mockcpp/mockcpp.hpp"
#include "gtest/gtest.h"
#include <iostream>
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
