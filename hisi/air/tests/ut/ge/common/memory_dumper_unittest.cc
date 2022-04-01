/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <gtest/gtest.h>
#define protected public
#define private public
#include "common/debug/memory_dumper.h"
#include "common/dump/exception_dumper.h"
#undef private
#undef protected
namespace ge {
class UtestMemoryDumper : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestMemoryDumper, OpenFile_success) {
MemoryDumper memory_dumper;
const std::string dump_file_path = "./test_file";
EXPECT_NE(MemoryDumper::OpenFile(dump_file_path), -1);
}



}