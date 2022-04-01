/**
 * @file adx_dump_hdc_helper.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_DUMP_HDC_HELPER_H
#define ADX_DUMP_HDC_HELPER_H
#include <cstdint>
#include "common/singleton.h"
#include "ide_daemon_api.h"
#include "commopts/adx_comm_opt_manager.h"
#include "commopts/hdc_comm_opt.h"
#include "adx_dump_record.h"
namespace Adx {
class AdxDumpHdcHelper : public Adx::Common::Singleton::Singleton<AdxDumpHdcHelper> {
public:
    AdxDumpHdcHelper();
    ~AdxDumpHdcHelper();
    bool Init();
    void UnInit();
    IdeErrorT ParseConnectInfo(const std::string &privInfo, std::map<std::string, std::string> &proto);
    IdeErrorT HandShake(const std::string &info, IDE_SESSION &session);
    IdeErrorT DataProcess(const IDE_SESSION &session, const IdeDumpChunk &dumpChunk);
    IdeErrorT Finish(IDE_SESSION &session);
private:
    bool init_;
    CommHandle client_;
};
IDE_SESSION HdcDumpStart(IdeString privInfo);
IdeErrorT HdcDumpData(IDE_SESSION session, const IdeDumpChunk *dumpChunk);
IdeErrorT HdcDumpEnd(IDE_SESSION session);
}
#endif