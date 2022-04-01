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

#ifndef DGE_CLIENT_H
#define DGE_CLIENT_H
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <list>
#include "aicpu/queue_schedule/qs_client.h"

namespace bqs {
// max ip len
constexpr size_t MAX_IP_LEN = 16UL;
// max tag name len
constexpr size_t MAX_TAG_NAME_LEN = 128UL;

// config command for datagw
enum class ConfigCmd : int32_t {
  DGW_CFG_CMD_BIND_ROUTE = 0,
  DGW_CFG_CMD_UNBIND_ROUTE = 1,
  DGW_CFG_CMD_QRY_ROUTE = 2,
  DGW_CFG_CMD_ADD_GROUP = 3,
  DGW_CFG_CMD_DEL_GROUP = 4,
  DGW_CFG_CMD_QRY_GROUP = 5,
  DGW_CFG_CMD_RESERVED = 6,
};

// endpoint type
enum class EndpointType : int32_t {
  QUEUE = 0,
  NAMED_COMM_CHANNEL = 1,
  GROUP = 2,
};

// endpoint status
enum class EndpointStatus : int32_t {
  AVAILABLE = 0,
  UNAVAILABLE = 1,
};

// route status
enum class RouteStatus : int32_t {
  ACTIVE = 0,
  INACTIVE = 1,
  ERROR = 2,
};

// group policy
enum class GroupPolicy : int32_t {
  HASH = 0,
  BROADCAST = 1,
};

// Query mode
enum class QueryMode : int32_t {
  DGW_QUERY_MODE_SRC_ROUTE = 0,
  DGW_QUERY_MODE_DST_ROUTE = 1,
  DGW_QUERY_MODE_SRC_DST_ROUTE = 2,
  DGW_QUERY_MODE_ALL_ROUTE = 3,
  DGW_QUERY_MODE_GROUP = 4,
  DGW_QUERY_MODE_RESERVED = 5,
};

// qs scheduling policy bitmap
enum class SchedPolicy : uint64_t {
  POLICY_UNSUB_F2NF = 1UL,
  POLICY_SUB_BUF_EVENT = 2UL,
};

#pragma pack(push, 1)
// queue attr : can not named QueueAttr, duplicatable name with driver
struct FlowQueueAttr {
  int32_t queueId;  // queue id
};

// named comm channel attr
struct NamedChannelAttr {
  char localIp[MAX_IP_LEN];     // local ip
  uint32_t localPort;           // local port
  char peerIp[MAX_IP_LEN];      // peer ip
  uint32_t peerPort;            // peer port
  char name[MAX_TAG_NAME_LEN];  // tag name
};

// group attr
struct GroupAttr {
  int32_t groupId;    // group id
  GroupPolicy policy;
  uint32_t endpointNum;
};

struct Endpoint {
  EndpointType type;
  EndpointStatus status;
  char rsv[32];
  union {
    FlowQueueAttr queueAttr;
    NamedChannelAttr namedChannelAttr;
    GroupAttr groupAttr;
  } attr;
};

// group query
struct GroupQuery {
  uint32_t endpointNum;
  int32_t groupId;
};

// route query
struct RouteQuery {
  uint32_t routeNum;
  Endpoint src;
  Endpoint dst;
};

struct ConfigQuery {
  QueryMode mode;
  union {
    GroupQuery groupQuery;
    RouteQuery routeQuery;
  } qry;
};

struct GroupConfig {
  int32_t groupId;
  uint32_t endpointNum;
  Endpoint *endpoints;
};

struct Route {
  RouteStatus status;
  Endpoint src;
  Endpoint dst;
  char rsv[32];
};

struct RouteConfig {
  uint32_t routeNum;
  Route *routes;
};

struct ConfigInfo {
  ConfigCmd cmd;
  union {
    GroupConfig groupCfg;
    RouteConfig routesCfg;
  } cfg;
};

struct IdentityInfo {
  uint64_t transId = 0UL;
  char rsv[56];
};

#pragma pack(pop)

class __attribute((visibility("default"))) DgwClient {
 public:
  static std::shared_ptr<DgwClient> GetInstance(const uint32_t deviceId);
  int32_t Initialize(const uint32_t dgwPid, const std::string procSign);
  int32_t Finalize();
  int32_t CreateHcomHandle(const std::string masterIp, const uint32_t masterPort,
                           const std::string localIp, const uint32_t localPort,
                           const std::string remoteIp, const uint32_t remotePort,
                           uint64_t &handle);

  int32_t DestroyHcomHandle(const uint64_t handle);
  int32_t CreateHcomTag(const uint64_t handle, const std::string tagName, int32_t &tag);
  int32_t DestroyHcomTag(const uint64_t handle, const int32_t tag);

  int32_t BindQueRoute(const std::vector<QueueRoute> &qRoutes, std::vector<int32_t> &bindResults);
  int32_t UnbindQueRoute(const std::vector<QueueRoute> &qRoutes, std::vector<int32_t> &results);
  int32_t QueryQueRoute(const QueueRouteQuery &dgwQuery, std::vector<QueueRoute> &routes);
  int32_t QueryQueRouteNum(const QueueRouteQuery &dgwQuery, int32_t &routeNum);

  explicit DgwClient(const uint32_t deviceId);
  ~DgwClient() = default;

 public:
  int32_t UpdateConfig(ConfigInfo &cfgInfo, std::vector<int32_t> &cfgRets) const;
  int32_t QueryConfigNum(ConfigQuery &query) const;
  int32_t QueryConfig(const ConfigQuery &query, ConfigInfo &cfgInfo) const;

 private:
  struct ConfigParams {
    ConfigQuery *query;
    ConfigInfo *cfgInfo;
    size_t cfgLen;
    size_t totalLen;
  };
};
}
#endif // DGW_CLIENT_H
