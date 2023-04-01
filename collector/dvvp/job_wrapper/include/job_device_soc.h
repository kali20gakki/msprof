/**
* @file job_soc.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ANALYSIS_DVVP_JOB_DEVICE_SOC_H
#define ANALYSIS_DVVP_JOB_DEVICE_SOC_H

#include "collection_register.h"
#include "job_adapter.h"
#include "uploader_mgr.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
class JobDeviceSoc : public JobAdapter {
public:
    explicit JobDeviceSoc(int devIndexId);
    ~JobDeviceSoc() override;

public:
    int StartProf(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params) override;
    int StopProf(void) override;

private:
    int CreateCollectionJobArray();
    int SetCollectionJobCfg();
    int CreateDeviceCollectionJobArray();
    int CreateHostCollectionJobArray();
    int RegisterCollectionJobs() const;
    void UnRegisterCollectionJobs();
    void StoreData(const std::string &path, const std::string &fileName);
    int SendData(const std::string &filePath, const std::string &data);
    int ParseTsCpuConfig(SHARED_PTR_ALIA<PMUEventsConfig> cfg);
    int ParseAiCoreConfig(SHARED_PTR_ALIA<PMUEventsConfig> cfg);
    int ParseAivConfig(SHARED_PTR_ALIA<PMUEventsConfig> cfg);
    int ParseControlCpuConfig(SHARED_PTR_ALIA<PMUEventsConfig> cfg);
    int ParseLlcConfig(SHARED_PTR_ALIA<PMUEventsConfig> cfg);
    int ParseDdrCpuConfig(SHARED_PTR_ALIA<PMUEventsConfig> cfg);
    int ParsePmuConfig(SHARED_PTR_ALIA<PMUEventsConfig> cfg);
    int ParseHbmConfig(SHARED_PTR_ALIA<PMUEventsConfig> cfg);
    std::string GenerateFileName(const std::string &fileName);
    std::string GenerateDurationdata();
    int StartProfHandle(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);

private:
    int devIndexId_;
    bool isStarted_;
    std::string tmpResultDir_;
    std::string jobId_;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params_;
    SHARED_PTR_ALIA<CollectionJobCommonParams> collectionjobComnCfg_;
    std::array<CollectionJobT, NR_MAX_COLLECTION_JOB> CollectionJobV_;
};
}}}
#endif
