/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
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
    int CreateDeviceCollectionTsJobArray();
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
