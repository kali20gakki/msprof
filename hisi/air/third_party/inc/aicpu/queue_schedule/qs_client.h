/**
 * Copyright 2020 Huawei Technologies Co., Ltd

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#ifndef QUEUE_SCHEDULE_QS_CLIENT_H
#define QUEUE_SCHEDULE_QS_CLIENT_H
#include <vector>
#include <mutex>

namespace bqs {
using char_t = char;
enum BqsClientErrorCode : int32_t {
  BQS_OK = 0,
  BQS_PARAM_INVALID = 1,
  BQS_INNER_ERROR = 2,
  BQS_DRIVER_ERROR = 3,
  BQS_EASY_COMM_ERROR = 4,
  BQS_PROTOBUF_ERROR = 5
};

enum QueueSubEventType {
  CREATE_QUEUE,
  DESTROY_QUEUE,
  DE_QUEUE,
  EN_QUEUE,
  PEEK_QUEUE,
  GRANT_QUEUE,
  ATTACH_QUEUE,
  DRIVER_PROCESS_SPLIT = 1024,
  AICPU_BIND_QUEUE,
  AICPU_BIND_QUEUE_RES,
  AICPU_BIND_QUEUE_INIT,
  AICPU_BIND_QUEUE_INIT_RES,
  AICPU_UNBIND_QUEUE,
  AICPU_UNBIND_QUEUE_RES,
  AICPU_QUERY_QUEUE,
  AICPU_QUERY_QUEUE_RES,
  AICPU_QUERY_QUEUE_NUM,
  AICPU_QUERY_QUEUE_NUM_RES,
  AICPU_QUEUE_RELATION_PROCESS,
  AICPU_RELATED_MESSAGE_SPLIT = 2048,
  ACL_BIND_QUEUE,
  ACL_BIND_QUEUE_INIT,
  ACL_UNBIND_QUEUE,
  ACL_QUERY_QUEUE,
  ACL_QUERY_QUEUE_NUM,
  ACL_QUEUE_RELATION_PROCESS,
  DGW_CREATE_HCOM_HANDLE,
  DGW_CREATE_HCOM_TAG,
  DGW_DESTORY_HCOM_TAG,
  DGW_DESTORY_HCOM,
};

typedef struct _BQSBindQueueItem {
  uint32_t srcQueueId_;
  uint32_t dstQueueId_;
} BQSBindQueueItem;

typedef struct _BQSbindQueueResult {
  int32_t bindResult_;
} BQSBindQueueResult;

enum QsQueryType {
  BQS_QUERY_TYPE_SRC,
  BQS_QUERY_TYPE_DST,
  BQS_QUERY_TYPE_SRC_AND_DST,
  BQS_QUERY_TYPE_SRC_OR_DST
};

enum EventGroupId {
  ENQUEUE_GROUP_ID = 5,
  F2NF_GROUP_ID,
  BIND_QUEUE_GROUP_ID,
};

#pragma  pack(push, 1)

struct QsBindInit {
  uint64_t syncEventHead;
  int32_t pid;
  uint32_t grpId;
  char_t rsv[24];
};

struct QsRouteHead {
  uint32_t length;
  uint32_t routeNum;
  uint32_t subEventId;
  uint64_t userData;
};

struct QueueRoute {
  uint32_t srcId;
  uint32_t dstId;
  int32_t status;
  int16_t srcType;
  int16_t dstType;
  uint64_t srcHandle;
  uint64_t dstHandle;
  char_t rsv[8];
};

struct QueueRouteList {
  uint64_t syncEventHead;
  uint64_t routeListMsgAddr;
  char_t rsv[24];
};

struct QueueRouteQuery {
  uint64_t syncEventHead;
  uint32_t queryType;
  uint32_t srcId;
  uint32_t dstId;
  int16_t srcType;
  int16_t dstType;
  uint64_t routeListMsgAddr;
  char_t rsv[8];
};

struct QsProcMsgRsp {
  uint64_t syncEventHead;
  int32_t retCode;
  uint32_t retValue;
  uint16_t majorVersion;
  uint16_t minorVersion;
  char_t rsv[20];
};

struct CreateHcomInfo {
  char_t masterIp[32];
  uint32_t masterPort;
  char_t localIp[32];
  uint32_t localPort;
  char_t remoteIp[32];
  uint32_t remotePort;
  uint64_t handle;
};

struct CreateHcomTagInfo {
  uint64_t handle;
  char_t tagName[128];
};

struct HcomHandleAndTag {
  uint64_t handle;
  int32_t tag;
};

#pragma pack(pop)
typedef struct _BQSQueryPara {
  QsQueryType keyType_;
  BQSBindQueueItem bqsBindQueueItem_;
} BQSQueryPara;

typedef void (*ExeceptionCallback)(int32_t fd, void *data);

class __attribute__((visibility("default"))) BqsClient {
 public:
  static BqsClient *GetInstance(const char_t *const serverProcName, const uint32_t procNameLen,
                                const ExeceptionCallback fn);
  uint32_t BindQueue(const std::vector<BQSBindQueueItem> &bindQueueVec, std::vector<BQSBindQueueResult> &bindResultVec);
  uint32_t UnbindQueue(const std::vector<BQSQueryPara> &bqsQueryParaVec, std::vector<BQSBindQueueResult> &bindResultVec) const;
  uint32_t GetBindQueue(const BQSQueryPara &queryPara, std::vector<BQSBindQueueItem> &bindQueueVec) const;
  uint32_t GetPagedBindQueue(const uint32_t offset, const uint32_t limit, std::vector<BQSBindQueueItem> &bindQueueVec, uint32_t &total) const;
  uint32_t GetAllBindQueue(std::vector<BQSBindQueueItem> &bindQueueVec) const;
  int32_t Destroy() const;

 private:
  BqsClient();
  ~BqsClient();

  BqsClient(const BqsClient &) = delete;
  BqsClient(BqsClient &&) = delete;
  BqsClient &operator = (const BqsClient &) = delete;
  BqsClient &operator = (BqsClient &&) = delete;

 private:
  static std::mutex mutex_;
  static int32_t clientFd_;
  static bool initFlag_;
};
}
#endif // BUFFER_QUEUE_SCHEDULE_QS_CLIENT_H_
