/**
 * @file adx_dump_record.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_DUMP_RECORE_H
#define ADX_DUMP_RECORE_H
#include <mutex>
#include <string>
#include <cstdint>
#include "bound_queue.h"
#include "common/singleton.h"
#include "adx_msg_proto.h"
#include "adx_dump_process.h"
namespace Adx {
enum class RecordType {
    RECORD_INVALID,
    RECORD_QUEUE,
    RECORD_FILE,
};

struct UihostDumpDataInfo {
    bool isEndFlag;
    std::string pathInUihost;
    std::string pathInHost;
};

struct HostDumpDataInfo {
    std::shared_ptr<MsgProto> msg;
    uint32_t recvLen;
};

bool SendDumpMsgToRemote(const std::string &msg, uint16_t flag);
class AdxDumpRecord : public Adx::Common::Singleton::Singleton<AdxDumpRecord> {
public:
    AdxDumpRecord();
    virtual ~AdxDumpRecord();
    int32_t GetDumpInitNum() const;
    void UpdateDumpInitNum(bool isPlus);
    int32_t Init(const std::string &hostPid);
    int32_t UnInit();
    void SetWorkPath(const std::string &path);
    void RecordDumpInfo();
    bool RecordDumpDataToQueue(HostDumpDataInfo &info);
    bool DumpDataQueueIsEmpty() const;
    AdxDumpRecord(AdxDumpRecord const &) = delete;
    AdxDumpRecord &operator=(AdxDumpRecord const &) = delete;

private:
    bool RecordDumpDataToDisk(const DumpChunk &dumpChunk) const;
    bool JudgeRemoteFalg(const std::string &msg) const;
private:
    bool dumpRecordFlag_;
    std::string dumpPath_;
    std::string workPath_;
    BoundQueue<HostDumpDataInfo> hostDumpDataInfoQueue_;
    int32_t dumpInitNum_;
};
}
#endif
