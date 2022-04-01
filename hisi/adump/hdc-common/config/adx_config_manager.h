/**
 * @file adx_config_manager.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef ADC_CONFIG_MANAGER_H
#define ADC_CONFIG_MANAGER_H
#include <string>
#include <vector>
#include <memory>
#include "common/singleton.h"
#include "config/config_utils.h"
#include "file_utils.h"
#define FILE_CHECK_EXIST(path, action) do {                       \
    if (!Adx::FileUtils::IsFileExist(path)) {                     \
        IDE_LOGW("verify files isn't exists");                    \
        action;                                                   \
    }                                                             \
}while (0)

namespace Adx {
namespace Manager {
namespace Config {
using namespace Adx::Common::Config;
using NetAdapterIpAddr = Adx::Common::Config::IfIpInfo;
class AdxConfigManager : public Adx::Common::Singleton::Singleton<AdxConfigManager> {
public:
    AdxConfigManager();
    virtual ~AdxConfigManager();
    bool Init();
    bool GetDefaultTime() const;
    void SetDefaultTime(bool value);
    bool GetSockSwitch() const;
    void SetSockSwitch(bool value);
    int GetHostPort() const;
    void SetHostPort(int value);
    std::string GetTlsOption();
    void SetTlsOption(const std::string &value);
    unsigned long GetCertExprie() const;
    void SetCertExprie(unsigned long value);
    std::string GetCertCipher();
    void SetCertCipher(const std::string &value);
    std::string GetWorkPath();
    std::string GetAdtmpPath();
    void SetWorkPath(const std::string &path);
    void SetCertPath(const std::string &path);
    std::string GetLogPath();
    void SetLogPath(const std::string &path);
    std::string GetCfgPath();
    std::string GetSelfPath();
    std::string GetSecuValue();
    void SetSecuValue(const std::string &value);
    std::string GetStoreValue();
    void SetStoreValue(const std::string &value);
    std::vector<NetAdapterIpAddr> GetHostIpInfo();
    void SetHostIpInfo(const std::vector<NetAdapterIpAddr> &info);
    void UpdateConfig();
    int GetInstallMode() const;
    void SetInstallMode(const int &mode);
private:
    bool CheckAndCreatePath(const std::string &value) const;
    bool CheckVerifyFileExist(const std::string &verPath) const;
    bool SearchCurrVerifyPath(std::string &path);
private:
    bool defaultTime_;
    bool sockSwitch_;
    int installMode_;
    int netPort_;
    unsigned long certExprie_;
    std::string tlsOption_;
    std::string certCipher_;
    std::string workPath_;
    std::string adtmpPath_;
    std::string rworkPath_;
    std::string certPath_;
    std::string logPath_;
    std::string cfgPath_;
    std::string selfPath_;
    std::string secuValue_;
    std::string storeValue_;
    std::vector<NetAdapterIpAddr> netIfInfo_;
    std::unique_ptr<Adx::Common::Config::Config> config_;
};
}
}
}
#endif