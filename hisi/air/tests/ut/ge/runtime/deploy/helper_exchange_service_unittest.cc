/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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
#include <gmock/gmock.h>
#include "securec.h"

#include "ge_inner_error_codes.h"
#define protected public
#define private public
#include "common/data_flow/queue/helper_exchange_service.h"
#undef private
#undef protected
#include "runtime/rt_mem_queue.h"
#include "exec_runtime/runtime_tensor_desc.h"

using namespace ::testing;

namespace ge {
class HelperExchangeServiceTest : public testing::Test {
 protected:
  void SetUp() override {
  }

  void TearDown() override {
  }
};

TEST_F(HelperExchangeServiceTest, TestCreateQueue) {
  HelperExchangeService exchange_service;
  uint32_t queue_id = 0;
  ASSERT_EQ(exchange_service.CreateQueue(0, "queue", 2, RT_MQ_MODE_PULL, queue_id), SUCCESS);
}

TEST_F(HelperExchangeServiceTest, TestDestroyQueue) {
  HelperExchangeService exchange_service;
  ASSERT_EQ(exchange_service.DestroyQueue(0, 2), SUCCESS);
}

TEST_F(HelperExchangeServiceTest, TestEnqueueAndDequeue) {
  HelperExchangeService exchange_service;
  uint32_t queue_id = 0;
  ASSERT_EQ(exchange_service.CreateQueue(0, "queue", 2, RT_MQ_MODE_PULL, queue_id), SUCCESS);
  uint8_t buf[128];
  ASSERT_EQ(exchange_service.Enqueue(0, queue_id, buf, sizeof(buf)), SUCCESS);
  ControlInfo control_Info;
  ASSERT_EQ(exchange_service.Dequeue(0, queue_id, buf, sizeof(buf), control_Info), SUCCESS);
}

TEST_F(HelperExchangeServiceTest, TestDequeueEmpty) {
  HelperExchangeService exchange_service;
  uint32_t queue_id = 0;
  ASSERT_EQ(exchange_service.CreateQueue(0, "queue", 2, RT_MQ_MODE_PULL, queue_id), SUCCESS);
  uint8_t buf[128];
  ControlInfo control_Info;
  ASSERT_NE(exchange_service.Dequeue(0, queue_id, buf, sizeof(buf), control_Info), SUCCESS);
}

TEST_F(HelperExchangeServiceTest, TestProcessEmptyToNotEmptyEvent) {
  HelperExchangeService exchange_service;
  uint32_t queue_id = 0;
  exchange_service.ProcessEmptyToNotEmptyEvent(queue_id);
}

TEST_F(HelperExchangeServiceTest, TestProcessF2NFEvent) {
  HelperExchangeService exchange_service;
  uint32_t queue_id = 0;
  exchange_service.ProcessF2NFEvent(queue_id);
}
}  // namesapce ge