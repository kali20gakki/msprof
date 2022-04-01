/**
 * @file config_utils.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_COMMON_UTILS_CONFIG_H
#define ADX_COMMON_UTILS_CONFIG_H
#include <vector>
#include <memory>
#include <string>
#include "common/config.h"
#include "log/hdc_log.h"
namespace Adx {
namespace Common {
namespace Config {
/* config default network value */
const int DEFAULT_HOST_PORT                          = 22118;  // default aihost socket port
const int DEFAULT_INSTALL_MODE                       = 0;      // install mode
/* dump default value */
const unsigned long DEFAULT_CERT_EXPIRE_DAY          = 60;     // default 60 days
const std::string DEFAULT_WORK_PATH                  = "~/";
const std::string DEFAULT_LOG_PATH                   = "ide_daemon/log/";
const std::string DEFAULT_ADTMP_PATH                 = "ide_daemon/";
const std::string DEFAULT_CFG_PATH                   = "~/ide_daemon/";
struct IfIpInfo {
    std::string ifName; // the name of network interface
    std::string ifAddr; // ip address
};

class ConfigParse {
public:
    explicit ConfigParse(const std::string &path);
    ~ConfigParse();
    bool GetDigitValueByKey(const std::string &key, std::vector<std::string> &cfgValue) const;
    bool GetValueByKey(const std::string &key, int &value) const;
    bool GetValueByKey(const std::string &key, unsigned long &value) const;
    bool GetValueByKey(const std::string &key, std::string &value) const;
    bool ParseConfigByKey(const std::string &key, std::vector<std::string> &value) const;
protected:
    std::string configPath_;
};

class ConfigParseFactory {
public:
    virtual ~ConfigParseFactory() {}
    static std::unique_ptr<ConfigParse> MakeConfigParse(const std::string &type, const std::string &path);
};

using NaAdapterIpAddr = IfIpInfo;
class Config {
public:
    Config() : parse_(nullptr) {}
    virtual ~Config() {}
    virtual bool GetDefaultTime() const = 0;
    virtual bool GetSockSwitch() const = 0;
    virtual int GetHostPort() const = 0;
    virtual int GetInstallMode() const = 0;
    virtual unsigned long GetCertExprie() const = 0;
    virtual std::string GetCertCipher() = 0;
    virtual std::string GetTlsOption() = 0;
    virtual std::string GetWorkPath() = 0;
    virtual std::string GetLogPath() = 0;
    virtual std::string GetSecuValue() = 0;
    virtual std::string GetStoreValue() = 0;
    virtual std::vector<NaAdapterIpAddr> GetHostIpInfo() = 0;
protected:
    virtual bool CheckPathValid(std::string &path) = 0;
protected:
    std::unique_ptr<ConfigParse> parse_;
};

class CfgConfig : public Config {
public:
    explicit CfgConfig(std::unique_ptr<ConfigParse> parse);
    ~CfgConfig() override;
    bool GetDefaultTime() const override;
    bool GetSockSwitch() const override;
    int GetHostPort() const override;
    int GetInstallMode() const override;
    unsigned long GetCertExprie() const override;
    std::string GetCertCipher() override;
    std::string GetTlsOption() override;
    std::string GetWorkPath() override;
    std::string GetLogPath() override;
    std::string GetSecuValue() override;
    std::string GetStoreValue() override;
    std::vector<NaAdapterIpAddr> GetHostIpInfo() override;
private:
    bool CheckPathValid(std::string &path) override;
    std::string GetValidPathByKey(const std::string &key, const std::string &defaultPath);
private:
    std::string workPath_;
};
}
}
}
#endif
