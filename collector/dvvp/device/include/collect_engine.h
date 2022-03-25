/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_DEVICE_COLLECTION_ENGINE_H
#define ANALYSIS_DVVP_DEVICE_COLLECTION_ENGINE_H

#include <memory>
#include <string>
#include <vector>
#include "ai_drv_dev_api.h"
#include "ai_drv_prof_api.h"
#include "message/prof_params.h"
#include "prof_channel.h"
#include "prof_msg_handler.h"
#include "prof_peripheral_job.h"
#include "prof_timer.h"
#include "transport/transport.h"

namespace analysis {
namespace dvvp {
namespace device {
class CollectEngine {
public:
    CollectEngine();
    virtual ~CollectEngine();

public:
    int Init(int devId = -1);
    int Uinit();
    void SetDevIdOnHost(int devIdOnHost);

    int CollectStart(const std::string &sampleConfig,
                     analysis::dvvp::message::StatusInfo &status);

    int CollectStop(analysis::dvvp::message::StatusInfo &status,
                    bool isReset = false);

    int CollectStartReplay(SHARED_PTR_ALIA<std::vector<std::string> > ctrlCpuEvent,
                           SHARED_PTR_ALIA<std::vector<std::string> > tsCpuEvent,
                           SHARED_PTR_ALIA<std::vector<std::string> > aiCoreEvent,
                           SHARED_PTR_ALIA<std::vector<int> > aiCoreEventCores,
                           analysis::dvvp::message::StatusInfo &status,
                           SHARED_PTR_ALIA<std::vector<std::string> > llcEvent,
                           SHARED_PTR_ALIA<std::vector<std::string> > ddrEvent,
                           SHARED_PTR_ALIA<std::vector<std::string> > aivEvent,
                           SHARED_PTR_ALIA<std::vector<int> > aivEventCores);
    int CollectRegister(analysis::dvvp::message::StatusInfo &status);
    int CollectStopReplay(analysis::dvvp::message::StatusInfo &status);
    int CollectStopJob(analysis::dvvp::message::StatusInfo &status);

private:
    int SendFiles(const std::string &path, const std::string &destDir);
    std::string GetEventsStr(const std::vector<std::string> &events, const std::string &separator = ",");
    int CreateTmpDir(std::string &tmp);
    int CleanupResults();
    std::string BindFileWithChannel(const std::string &fileName, unsigned int channelId);
    void CreateCollectionJobArray();
    void SetCollectJobParam(const std::string &projectDir);
    void SetCollectL2CacheJobParam(const std::string &projectDir);
    void SetCollectFmkJobParam(const std::string &projectDir);
    void SetCollectHwTsJobParam(const std::string &projectDir);
    void SetCollectHwTs1JobParam(const std::string &projectDir);
    void SetCollectHbmJobParam(const std::string &projectDir);
    void SetCollectDdrJobParam(const std::string &projectDir);
    void SetCollectLlcJobParam(const std::string &projectDir);
    void SetCollectTsFwJobParam(const std::string &projectDir);
    void SetCollectTs1FwJobParam(const std::string &projectDir);
    void SetCollectAiCoreJobParam(const std::string &projectDir);
    void SetCollectAivJobParam(const std::string &projectDir);
    void SetCollectTsCpuJobParam(const std::string &projectDir);
    void SetCollectControlCpuJobParam(const std::string &projectDir);
    int InitBeforeCollectStart(const std::string &sampleConfig,
        analysis::dvvp::message::StatusInfo &status, std::string &projectDir);

    void InitTsCpuBeforeStartReplay(SHARED_PTR_ALIA<std::vector<std::string> > tsCpuEvent);
    void InitAiCoreBeforeStartReplay(SHARED_PTR_ALIA<std::vector<std::string> > aiCoreEvent,
        SHARED_PTR_ALIA<std::vector<int> > aiCoreEventCores);
    void InitAivBeforeStartReplay(SHARED_PTR_ALIA<std::vector<std::string> > aivEvents,
        SHARED_PTR_ALIA<std::vector<int> > aivEventCores);

    void StoreData(const std::string &path, const std::string &fileName);
    void InitFileChunkOfStoreData(
        SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunk, const std::string &fileName);
    int CheckPmuEventIsValid(SHARED_PTR_ALIA<std::vector<std::string> > ctrlCpuEvent,
        SHARED_PTR_ALIA<std::vector<std::string> > tsCpuEvent,
        SHARED_PTR_ALIA<std::vector<std::string> > aiCoreEvent,
        SHARED_PTR_ALIA<std::vector<int> > aiCoreEventCores,
        SHARED_PTR_ALIA<std::vector<std::string> > llcEvent,
        SHARED_PTR_ALIA<std::vector<std::string> > ddrEvent,
        SHARED_PTR_ALIA<std::vector<std::string> > aivEvent,
        SHARED_PTR_ALIA<std::vector<int> > aivEventCores);
    int CheckAiCoreEventIsValid(SHARED_PTR_ALIA<std::vector<std::string> > aiCoreEvent,
        SHARED_PTR_ALIA<std::vector<int> > aiCoreEventCores);
    int CheckAivEventIsValid(SHARED_PTR_ALIA<std::vector<std::string> > aivEvent,
        SHARED_PTR_ALIA<std::vector<int> > aivEventCores);
    int CheckDeviceValid();

private:
    SHARED_PTR_ALIA<analysis::dvvp::transport::ITransport> _transport;
    std::string _sample_config;
    SHARED_PTR_ALIA<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams> collectionjobComnCfg_;
    std::array<Analysis::Dvvp::JobWrapper::CollectionJobT,
        Analysis::Dvvp::JobWrapper::NR_MAX_COLLECTION_JOB> CollectionJobV_;
    volatile bool _is_stop;
    bool isInited_;
    std::string tmpResultDir_;
    unsigned long long startMono_;
    unsigned long long startRealtime_;
    unsigned long long cntvct_;
    bool isAicoreSampleBased_;
    bool isAicoreTaskBased_;
    bool isAivSampleBased_;
    bool isAivTaskBased_;

    volatile bool _is_started;
private:
    static std::mutex staticMtx_;
};
}  // namespace device
}  // namespace dvvp
}  // namespace analysis

#endif
