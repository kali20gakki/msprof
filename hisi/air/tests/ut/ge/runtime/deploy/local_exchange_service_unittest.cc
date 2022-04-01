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
#include "runtime/deploy/local_exchange_service.h"
#include "runtime/rt_mem_queue.h"
#include "exec_runtime/runtime_tensor_desc.h"

using namespace ::testing;

namespace {
rtError_t MemQueuePeekStub(int32_t devId, uint32_t qid, size_t *bufLen, int32_t timeout) {
  *bufLen = 1024;
  return RT_ERROR_NONE;
}

class RtMemQueueMock {
 public:
  virtual ~RtMemQueueMock() = default;

  MOCK_METHOD1(MemQueueInit, rtError_t(int32_t));
  MOCK_METHOD3(MemQueueCreate, rtError_t(int32_t, const rtMemQueueAttr_t *, uint32_t *));
  MOCK_METHOD2(MemQueueDestroy, rtError_t(int32_t, uint32_t));
  MOCK_METHOD4(MemQueuePeek, rtError_t(int32_t, uint32_t, size_t * , int32_t));
  MOCK_METHOD4(MemQueueEnQueueBuff, rtError_t(int32_t, uint32_t, rtMemQueueBuff_t * , int32_t));
  MOCK_METHOD4(MemQueueDeQueueBuff, rtError_t(int32_t, uint32_t, rtMemQueueBuff_t * , int32_t));
};

RtMemQueueMock *g_stub;
}  // namespace

rtError_t rtMemQueueCreate(int32_t devId, const rtMemQueueAttr_t *queAttr, uint32_t *qid) {
  return g_stub->MemQueueCreate(devId, queAttr, qid);
}

rtError_t rtMemQueueDestroy(int32_t devId, uint32_t qid) {
  return g_stub->MemQueueDestroy(devId, qid);
}

rtError_t rtMemQueueInit(int32_t devId) {
  return g_stub->MemQueueInit(devId);
}

rtError_t rtMemQueuePeek(int32_t devId, uint32_t qid, size_t *bufLen, int32_t timeout) {
  return g_stub->MemQueuePeek(devId, qid, bufLen, timeout);
}

rtError_t rtMemQueueEnQueueBuff(int32_t devId, uint32_t qid, rtMemQueueBuff_t *inBuf, int32_t timeout) {
  return g_stub->MemQueueEnQueueBuff(devId, qid, inBuf, timeout);
}

rtError_t rtMemQueueDeQueueBuff(int32_t devId, uint32_t qid, rtMemQueueBuff_t *outBuf, int32_t timeout) {
  return g_stub->MemQueueDeQueueBuff(devId, qid, outBuf, timeout);
}

namespace ge {
class LocalExchangeServiceTest : public testing::Test {
 protected:
  void SetUp() override {
    g_stub = &mock;
  }

  void TearDown() override {
  }

  RtMemQueueMock mock;
};

TEST_F(LocalExchangeServiceTest, TestCreateQueueCheckNameFailed) {
  LocalExchangeService exchange_service;
  uint32_t queue_id = 0;
  std::string queue_name;
  queue_name.resize(128);
  ASSERT_EQ(exchange_service.CreateQueue(0, queue_name, 2U, RT_MQ_MODE_PULL, queue_id), PARAM_INVALID);
}

TEST_F(LocalExchangeServiceTest, TestCreateQueueInitFailed) {
  LocalExchangeService exchange_service;
  uint32_t queue_id = 0;
  EXPECT_CALL(mock, MemQueueInit).WillRepeatedly(Return(RT_FAILED));
  ASSERT_EQ(exchange_service.CreateQueue(0, "queue", 2, RT_MQ_MODE_PULL, queue_id), RT_FAILED);
}

TEST_F(LocalExchangeServiceTest, TestCreateQueue) {
  LocalExchangeService exchange_service;
  uint32_t queue_id = 0;
  EXPECT_CALL(mock, MemQueueInit).Times(1).WillOnce(Return(SUCCESS));
  EXPECT_CALL(mock, MemQueueCreate).WillOnce(Return(RT_FAILED)).WillOnce(Return(SUCCESS));
  ASSERT_EQ(exchange_service.CreateQueue(0, "queue", 2, RT_MQ_MODE_PULL, queue_id), RT_FAILED);
  ASSERT_EQ(exchange_service.CreateQueue(0, "queue", 2, RT_MQ_MODE_PULL, queue_id), SUCCESS);
}

TEST_F(LocalExchangeServiceTest, TestDestroyQueueu) {
  LocalExchangeService exchange_service;
  EXPECT_CALL(mock, MemQueueDestroy).WillOnce(Return(RT_FAILED)).WillOnce(Return(SUCCESS));
  ASSERT_EQ(exchange_service.DestroyQueue(0, 2), RT_FAILED);
  ASSERT_EQ(exchange_service.DestroyQueue(0, 2), SUCCESS);
}

TEST_F(LocalExchangeServiceTest, TestEnqueue) {
  LocalExchangeService exchange_service;
  EXPECT_CALL(mock, MemQueueEnQueueBuff).WillOnce(Return(RT_FAILED)).WillOnce(Return(SUCCESS));
  uint8_t buf[128];
  ASSERT_EQ(exchange_service.Enqueue(0, 0, buf, sizeof(buf)), RT_FAILED);
  ASSERT_EQ(exchange_service.Enqueue(0, 0, buf, sizeof(buf)), SUCCESS);
}

TEST_F(LocalExchangeServiceTest, TestEnqueueWithCallback) {
  LocalExchangeService exchange_service;
  EXPECT_CALL(mock, MemQueueEnQueueBuff).WillOnce(Return(SUCCESS));
  auto fill_func = [](void *buffer, size_t size) -> Status {
    return SUCCESS;
  };
  ASSERT_EQ(exchange_service.Enqueue(0, 0, 128, fill_func), SUCCESS);
}

TEST_F(LocalExchangeServiceTest, TestDequeueSizeNotMatch) {
  LocalExchangeService exchange_service;
  EXPECT_CALL(mock, MemQueuePeek).WillOnce(testing::Invoke(MemQueuePeekStub));
  uint8_t buf[128];
  ControlInfo control_Info;
  ASSERT_EQ(exchange_service.Dequeue(0, 0, buf, sizeof(buf), control_Info), INTERNAL_ERROR);
}

TEST_F(LocalExchangeServiceTest, TestDequeue) {
  LocalExchangeService exchange_service;
  EXPECT_CALL(mock, MemQueuePeek).WillOnce(Return(RT_FAILED)).WillRepeatedly(Return(SUCCESS));
  EXPECT_CALL(mock, MemQueueDeQueueBuff).WillOnce(Return(RT_FAILED)).WillOnce(Return(SUCCESS));
  uint8_t buf[128];
  ControlInfo control_Info;
  ASSERT_EQ(exchange_service.Dequeue(0, 0, buf, sizeof(buf), control_Info), RT_FAILED); // peek failed
  ASSERT_EQ(exchange_service.Dequeue(0, 0, buf, sizeof(buf), control_Info), RT_FAILED); // dequeue failed
  ASSERT_EQ(exchange_service.Dequeue(0, 0, buf, sizeof(buf), control_Info), SUCCESS);
}

rtError_t rtMemQueueDeQueueBuffMock(int32_t device, uint32_t qid, rtMemQueueBuff_t *outBuf, int32_t timeout) {
  RuntimeTensorDesc mbuf_tensor_desc;
  mbuf_tensor_desc.shape[0] = 4;
  mbuf_tensor_desc.shape[1] = 1;
  mbuf_tensor_desc.shape[2] = 1;
  mbuf_tensor_desc.shape[3] = 4;
  mbuf_tensor_desc.shape[4] = 4;
  mbuf_tensor_desc.dtype = static_cast<int64_t>(DT_INT64);
  mbuf_tensor_desc.data_addr = static_cast<int64_t>(reinterpret_cast<intptr_t>(outBuf->buffInfo->addr));
  if (memcpy_s(outBuf->buffInfo->addr, sizeof(RuntimeTensorDesc), &mbuf_tensor_desc, sizeof(RuntimeTensorDesc)) !=
      EOK) {
    printf("Failed to copy mbuf data, dst size:%zu, src size:%zu\n", outBuf->buffInfo->len, sizeof(RuntimeTensorDesc));
    return -1;
  }
  return 0;
}

rtError_t rtMemQueuePeekMock(int32_t device, uint32_t qid, size_t *bufLen, int32_t timeout) {
  *bufLen = sizeof(RuntimeTensorDesc) + 4U * 4U;
  return 0;
}

TEST_F(LocalExchangeServiceTest, TestDequeueTensor) {
  LocalExchangeService exchange_service;
  EXPECT_CALL(mock, MemQueuePeek).WillOnce(Return(RT_FAILED)).WillRepeatedly(testing::Invoke(rtMemQueuePeekMock));
  EXPECT_CALL(mock, MemQueueDeQueueBuff)
      .WillOnce(Return(RT_FAILED))
      .WillOnce(testing::Invoke(rtMemQueueDeQueueBuffMock));
  GeTensor tensor;
  ControlInfo control_Info;
  ASSERT_EQ(exchange_service.DequeueTensor(0, 0, tensor, control_Info), RT_FAILED);  // peek failed
  ASSERT_EQ(exchange_service.DequeueTensor(0, 0, tensor, control_Info), RT_FAILED);  // dequeue failed
  ASSERT_EQ(exchange_service.DequeueTensor(0, 0, tensor, control_Info), SUCCESS);
}
}  // namesapce ge