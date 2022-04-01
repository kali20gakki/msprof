#include <gtest/gtest.h>

#define protected public
#define private public
#include "graph/load/model_manager/task_info/super_kernel/super_kernel_factory.h"
#include "runtime/kernel.h"
#undef protected
#undef private

namespace ge {
class UtestSuperKernelFactory : public testing::Test {
 protected:
  void SetUp() {
    skt::SuperKernelFactory::GetInstance().Init();
  }

  void TearDown() {
    skt::SuperKernelFactory::GetInstance().Init();
  }
};

TEST_F(UtestSuperKernelFactory, test_fuse_kernels) {
  std::vector<const void *> stub_func_list = {(void *)0x1234, (void *)0x1111};
  std::vector<void *> args_addr_list = { (void*)0x1234, (void*)0x1111 };
  std::unique_ptr<skt::SuperKernel> h;

  Status ret = skt::SuperKernelFactory::GetInstance().FuseKernels(stub_func_list, args_addr_list, 32, h);
  EXPECT_EQ(ret, ge::SUCCESS);
}

}