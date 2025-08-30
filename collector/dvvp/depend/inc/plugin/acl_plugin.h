/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
 * Description: acl interface
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2025-08-27
 */
#ifndef ACL_PLUGIN_H
#define ACL_PLUGIN_H

#include "singleton/singleton.h"
#include "plugin_handle.h"

namespace Collector {
namespace Dvvp {
namespace Plugin {
using aclrtStream = void *;
using AclrtGetLogicDevIdByUserDevIdFunc = std::function<int(int32_t, int32_t*)>;
using AclrtProfTraceFunc = std::function<int(void *, int32_t, aclrtStream)>;
class AclPlugin : public analysis::dvvp::common::singleton::Singleton<AclPlugin> {
public:
    AclPlugin() : soName_("libascendcl.so"), loadFlag_(0) {}
    bool IsFuncExist(const std::string &funcName) const;
    void GetAllFunction();
    int32_t MsprofAclrtGetLogicDevIdByUserDevId(int32_t userDevId, int32_t *logicDevId);
    int32_t MsprofAclrtProfTrace(uint64_t indexId, uint64_t modelId, uint16_t tagId, aclrtStream stream);
private:
    std::string soName_;
    static SHARED_PTR_ALIA<PluginHandle> pluginHandle_;
    PTHREAD_ONCE_T loadFlag_;
    AclrtGetLogicDevIdByUserDevIdFunc aclrtGetLogicDevIdByUserDevIdFunc_ = nullptr;
    AclrtProfTraceFunc aclrtProfTraceFunc_ = nullptr;
private:
    void LoadAclSo();
};
}
}
}

#endif // ACL_PLUGIN_H