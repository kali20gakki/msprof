#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include "depends/mmpa/src/mmpa_stub.h"

#define protected public
#define private public
#include "exec_runtime/execution_runtime.h"
#undef protected
#undef private

#include "runtime/local_execution_runtime.h"

namespace ge {

namespace {
void *mock_handle = nullptr;
void *mock_method = nullptr;

Status InitializeHelperRuntime(const std::map<std::string, std::string> &options) {
  ExecutionRuntime::SetExecutionRuntime(std::make_shared<LocalExecutionRuntime>());
  return SUCCESS;
}

class MockMmpa : public MmpaStubApi {
 public:
  void *DlOpen(const char *file_name, int32_t mode) override {
    return mock_handle;
  }
  void *DlSym(void *handle, const char *func_name) override {
    return mock_method;
  }

  int32_t DlClose(void *handle) override {
    return 0;
  }
};
}  // namespace

class ExecutionRuntimeTest : public testing::Test {
 protected:
  void SetUp() {
    MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  }
  void TearDown() {
    ExecutionRuntime::FinalizeExecutionRuntime();
    MmpaStub::GetInstance().Reset();
    mock_handle = nullptr;
    mock_method = nullptr;
  }
};

TEST_F(ExecutionRuntimeTest, TestLoadHeterogeneousLib) {
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  ASSERT_EQ(ExecutionRuntime::LoadHeterogeneousLib(), FAILED); // error load so
  mock_handle = (void *)0xffffffff;
  ASSERT_EQ(ExecutionRuntime::LoadHeterogeneousLib(), SUCCESS);
}

TEST_F(ExecutionRuntimeTest, TestSetupHeterogeneousRuntime) {
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());
  const std::map<std::string, std::string> options;
  ASSERT_EQ(ExecutionRuntime::SetupHeterogeneousRuntime(options), FAILED); // error find init func
  ExecutionRuntime::handle_ = (void *)0xffffffff;
  mock_method = (void *)&InitializeHelperRuntime;
  ASSERT_EQ(ExecutionRuntime::SetupHeterogeneousRuntime(options), SUCCESS);
}

TEST_F(ExecutionRuntimeTest, TestInitAndFinalize) {
  ASSERT_FALSE(ExecutionRuntime::IsHeterogeneous());
  ExecutionRuntime::FinalizeExecutionRuntime();  // instance not set
  ASSERT_TRUE(ExecutionRuntime::GetInstance() == nullptr);
  MmpaStub::GetInstance().SetImpl(std::make_shared<MockMmpa>());

  mock_handle = (void *)0xffffffff;
  mock_method = (void *)&InitializeHelperRuntime;
  const std::map<std::string, std::string> options;
  ASSERT_EQ(ExecutionRuntime::InitHeterogeneousRuntime(options), SUCCESS); // error load so
  ASSERT_TRUE(ExecutionRuntime::handle_ != nullptr);
  ASSERT_TRUE(ExecutionRuntime::IsHeterogeneous());
  ASSERT_TRUE(ExecutionRuntime::GetInstance() != nullptr);
  ExecutionRuntime::FinalizeExecutionRuntime();  // instance not set

  ASSERT_TRUE(ExecutionRuntime::GetInstance() == nullptr);
  ASSERT_TRUE(ExecutionRuntime::handle_ == nullptr);
}
} // namespace ge