/**
 * @file adx_dump_soc_helper.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_DUMP_SOC_HELPER_H
#define ADX_DUMP_SOC_HELPER_H
#include <string>
#include <map>
#include <cstdint>
#include <atomic>
#include "common/singleton.h"
#include "ide_daemon_api.h"
#include "common/extra_config.h"
namespace Adx {
const IDE_SESSION DEFAULT_SOC_SESSION = (IDE_SESSION)0xFFFF0000;
class AdxDumpSocHelper : public Adx::Common::Singleton::Singleton<AdxDumpSocHelper> {
public:
    AdxDumpSocHelper();
    ~AdxDumpSocHelper();
    bool Init(const std::string &hostPid);
    void UnInit();
    IdeErrorT ParseConnectInfo(const std::string &privInfo);
    IdeErrorT HandShake(const std::string &info, IDE_SESSION &session);
    IdeErrorT DataProcess(const IDE_SESSION &session, const IdeDumpChunk &dumpChunk);
    IdeErrorT Finish(IDE_SESSION &session);
private:
    std::atomic_flag init_ = ATOMIC_FLAG_INIT;
};
IDE_SESSION SocDumpStart(IdeString privInfo);
IdeErrorT SocDumpData(IDE_SESSION session, const IdeDumpChunk *dumpChunk);
IdeErrorT SocDumpEnd(IDE_SESSION session);
}
#endif