/**
 * @file config_utils.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *
 */
#include <fstream>
#include <common/utils.h>
#include "file_utils.h"
#include "string_utils.h"
#include "config_utils.h"
namespace Adx {
namespace Common {
namespace Config {
static const std::string DEFAULT_TIME_CFG_KEY               = "DEFAULT_TIME";
static const std::string SOCK_SWITCH_CFG_KEY                = "SOCK_SWITCH";
static const std::string HOST_IP_CFG_KEY                    = "HOST_IP";
static const std::string HOST_PORT_CFG_KEY                  = "HOST_PORT";
static const std::string CERT_EXPIRE_CFG_KEY                = "CERT_EXPIRE";
static const std::string CERT_CIPHER_CFG_KEY                = "CERT_CIPHER";
static const std::string TLS_OPTION_CFG_KEY                 = "TLS_OPTION";
static const std::string WORK_PATH_CFG_KEY                  = "WORK_PATH";
static const std::string LOG_PATH_CFG_KEY                   = "LOG_PATH";
static const std::string KMC_SECU_CFG_KEY                   = "SECU";
static const std::string KMC_STORE_CFG_KEY                  = "STORE";
static const std::string CFG_VIRTUAL_IP                     = "VIRTUAL_IP";
static const std::string INSTALL_MODE                       = "INSTALL_MODE";

/* config default network value */
static const int MAX_SOCK_PORT                              = 25000;  // max socket port
static const int MIN_SOCK_PORT                              = 20000;  // min socket port
static const unsigned int MAX_CFG_PATH_LENGTH               = 1024;   // max cfg path length
/* dump default value */
static const unsigned long MAX_CERT_EXPIRE_DAY              = 180;    // max 180 days
static const std::string DEFAULT_CERT_CIPHERS               = "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256";
static const std::string DEFAULT_TLS_OPTION                 = "TLSv1.2,TLSv1.3";

ConfigParse::ConfigParse(const std::string &path)
{
    this->configPath_ = path;
    if (!this->configPath_.empty() &&
        this->configPath_.back() != IdeDaemon::Common::Config::OS_SPLIT) {
        this->configPath_ += IdeDaemon::Common::Config::OS_SPLIT_STR;
    }
}

ConfigParse::~ConfigParse()
{
}

/**
 * @brief get (string) value by key from config
 * @param [in] key : get value of key
 * @param [out] value : get value
 * @return : true : success
 *        false : failed
 */
bool ConfigParse::GetValueByKey(const std::string &key, std::string &value) const
{
    std::vector<std::string> cfgValue;
    bool ret = ParseConfigByKey(key, cfgValue);
    if (ret == false || cfgValue.empty()) {
        IDE_LOGW("config [%s] non value", key.c_str());
        return false;
    }

    value = cfgValue.front();
    IDE_LOGI("config [%s] string value [%s]", key.c_str(), value.c_str());
    return true;
}

/**
 * @brief get digital value by key from config
 * @param [in] key : get value of key
 * @param [out]  : get value, string
 * @return : true : success
 *        false : failed
 */
bool ConfigParse::GetDigitValueByKey(const std::string &key, std::vector<std::string> &cfgValue) const
{
    bool ret = ParseConfigByKey(key, cfgValue);
    if (ret == false || cfgValue.empty()) {
        return false;
    }

    if (!Adx::StringUtils::IsIntDigital(cfgValue.front())) {
        IDE_LOGW("config %s exists non digital %s", key.c_str(), cfgValue.front().c_str());
        return false;
    }
    return true;
}

/**
 * @brief get (int) value by key from config
 * @param [in] key : get value of key
 * @param [out] value : get value
 * @return : true : success
 *        false : failed
 */
bool ConfigParse::GetValueByKey(const std::string &key, int &value) const
{
    std::vector<std::string> cfgValue;
    if (GetDigitValueByKey(key, cfgValue) == false) {
        return false;
    }

    try {
        value = std::stoi(cfgValue.front());
    } catch (const std::exception& e) {
        IDE_LOGW("get %s exception %s", key.c_str(), e.what());
        return false;
    }

    return true;
}

/**
 * @brief get (unsigned long)value by key from config
 * @param [in] key : get value of key
 * @param [out] value : get value
 * @return : true : success
 *        false : failed
 */
bool ConfigParse::GetValueByKey(const std::string &key, unsigned long &value) const
{
    std::vector<std::string> cfgValue;
    if (GetDigitValueByKey(key, cfgValue) == false) {
        return false;
    }

    try {
        value = std::stoul(cfgValue.front());
    } catch (const std::exception& e) {
        IDE_LOGW("get %s exception %s", key.c_str(), e.what());
        return false;
    }

    return true;
}

/**
 * @brief get value of key from config
 * @param [in] key : get value of key
 * @return :  config value
 */
bool ConfigParse::ParseConfigByKey(const std::string &key, std::vector<std::string> &result) const
{
    std::string line = "";
    std::string value = "";
    std::string defPath = this->configPath_ + IdeDaemon::Common::Config::IDE_CFG_VALUE;
    std::ifstream fin(defPath);
    if (fin.is_open()) {
        while (getline(fin, line)) {
            line = IdeDaemon::Common::Utils::LeftTrim(line, " ");
            if (!line.empty() && line.front() == '#') {
                continue;
            }

            size_t pos = line.find(key); // find key
            if (pos == std::string::npos) {   // check is not npos
                continue;
            }

            size_t pattern_pos = line.find("=");
            if (pattern_pos == std::string::npos) {
                continue;
            }

            value = line.substr(pattern_pos + 1); // return the value
            value = IdeDaemon::Common::Utils::LeftTrim(value, " ");
            if (!value.empty()) {
                result.push_back(value);
            }
            break;
        }
    } else {
        IDE_LOGI("get key [%s] from config [%s] which isn't exist", key.c_str(), defPath.c_str());
        return false;
    }

    fin.close();
    return true;
}

/**
 * @brief create config prase by type
 * @param [in] type : config parse type by string
 * @param [in] path : config path
 * @return     nullptr : create config parse failed, != nullptr create success
 */
std::unique_ptr<ConfigParse> ConfigParseFactory::MakeConfigParse(const std::string &type, const std::string &path)
{
    std::unique_ptr<ConfigParse> cfg = nullptr;
    if (type == "cfg") {
        cfg = std::unique_ptr<ConfigParse>(new (std::nothrow)ConfigParse(path));
    }
    return cfg;
}

CfgConfig::CfgConfig(std::unique_ptr<ConfigParse> parse)
    : workPath_(Adx::Common::Config::DEFAULT_WORK_PATH)
{
    this->parse_.reset(parse.release());
}

CfgConfig::~CfgConfig()
{
}

/**
 * @brief get default time flag from cfg
 * @return : default flag (1) true : set default time (2019-01-01), false : use system time
 */
bool CfgConfig::GetDefaultTime() const
{
    int value = 0;
    if (this->parse_ != nullptr && this->parse_->GetValueByKey(DEFAULT_TIME_CFG_KEY, value)) {
        if (value == 1) {
            return true;
        }
    }

    return false;
}

/**
 * @brief  : get socket switch for network listening network form the config file
 * @return : false : socket close(0), true socket open(1)
 */
bool CfgConfig::GetSockSwitch() const
{
    int value = 0;
    if (this->parse_ != nullptr && this->parse_->GetValueByKey(SOCK_SWITCH_CFG_KEY, value)) {
        if (value == 1) {
            return true;
        }
    }

    return false;
}

/**
 * @brief  : get socket port form the config file
 * @return : get socket port[20000, 25000], default port (22118)
 */
int CfgConfig::GetHostPort() const
{
    int value = 0;
    if (this->parse_ != nullptr && this->parse_->GetValueByKey(HOST_PORT_CFG_KEY, value)) {
        if (value >= MIN_SOCK_PORT && value <= MAX_SOCK_PORT) {
            return value;
        }
    }

    return DEFAULT_HOST_PORT;
}

/**
 * @brief  : get ssl cert exprie form the config file
 * @return : cert exprie day [1, 180], default exprie 60 day
 */
unsigned long CfgConfig::GetCertExprie() const
{
    static unsigned long day = 0;
    unsigned long value = 0;
    if (this->parse_ != nullptr && this->parse_->GetValueByKey(CERT_EXPIRE_CFG_KEY, value)) {
        if (value > 0 && value <= MAX_CERT_EXPIRE_DAY) {
            return value;
        }
    }

    if (day != value) {
        day = value;
        IDE_LOGW("Exprie Validity Invalid Value %ld, Use Default %ld days", day, DEFAULT_CERT_EXPIRE_DAY);
    }
    return DEFAULT_CERT_EXPIRE_DAY;
}

/**
 * @brief  : get ssl cert cipher form the config file
 * @return : cert cipher "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256"
 */
std::string CfgConfig::GetCertCipher()
{
    std::string value = DEFAULT_CERT_CIPHERS;
    std::string res;
    if (this->parse_ != nullptr && this->parse_->GetValueByKey(CERT_CIPHER_CFG_KEY, value)) {
        std::vector<std::string> result = IdeDaemon::Common::Utils::Split(value, false, "", ",");
        size_t size = result.size();
        for (size_t i = 0; i < size; i++) {
            value = result[i];
            if (std::string(DEFAULT_CERT_CIPHERS).find(value) == std::string::npos) {
                continue;
            }
            if (res.empty()) {
                res = value;
            } else {
                res += ":" + value;
            }
        }
    }

    if (res.empty()) {
        IDE_LOGW("use default cipher because of %s unset CERT_CIPHER or not in whitelist So ", value.c_str());
        return DEFAULT_CERT_CIPHERS;
    }

    return res;
}

/**
 * @brief  : get tls option form the config file
 * @return : tls option : TLSv1.2, TLSv1.3
 */
std::string CfgConfig::GetTlsOption()
{
    std::string value = DEFAULT_TLS_OPTION;
    if (this->parse_ != nullptr && this->parse_->GetValueByKey(TLS_OPTION_CFG_KEY, value)) {
        if (value.find("TLSv1.2") != std::string::npos ||
            value.find("TLSv1.3") != std::string::npos) {
            return value;
        }
    }

    return DEFAULT_TLS_OPTION;
}

/**
 * @brief  : get work path form the config file
 * @return : WORK_PATH , default ~
 */
std::string CfgConfig::GetWorkPath()
{
    std::string defaultPath = DEFAULT_WORK_PATH;
    this->workPath_ = GetValidPathByKey(WORK_PATH_CFG_KEY, defaultPath);
    if (!this->workPath_.empty() && this->workPath_.back() != IdeDaemon::Common::Config::OS_SPLIT) {
        this->workPath_ += IdeDaemon::Common::Config::OS_SPLIT_STR;
    }
    return this->workPath_;
}

/**
 * @brief  : get log path form the config file
 * @return : LOG_PATH , default ~
 */
std::string CfgConfig::GetLogPath()
{
    std::string defaultPath = workPath_ + DEFAULT_LOG_PATH;
    return GetValidPathByKey(LOG_PATH_CFG_KEY, defaultPath);
}

/**
 * @brief  : get secu value form the config file
 * @return : SECU value of ide_daemon.cfg
 */
std::string CfgConfig::GetSecuValue()
{
    std::string value;
    if (this->parse_ != nullptr && this->parse_->GetValueByKey(KMC_SECU_CFG_KEY, value)) {
        if (!value.empty()) {
            return value;
        }
    }

    return value;
}

/**
 * @brief  : get store value form the config file
 * @return : STORE value of ide_daemon.cfg
 */
std::string CfgConfig::GetStoreValue()
{
    std::string value;
    if (this->parse_ != nullptr && this->parse_->GetValueByKey(KMC_STORE_CFG_KEY, value)) {
        if (!value.empty()) {
            return value;
        }
    }

    return value;
}

/**
 * @brief  : get host ip form the config file
 * @return : HOST_IP value of ide_daemon.cfg
 */
std::vector<NaAdapterIpAddr> CfgConfig::GetHostIpInfo()
{
    std::vector<NaAdapterIpAddr> result;
    NaAdapterIpAddr info;
    if (this->parse_ == nullptr) {
        return result;
    }
    std::vector<std::string> value;
    bool ret = this->parse_->ParseConfigByKey(HOST_IP_CFG_KEY, value);
    if (ret && !value.empty()) {
        for (std::vector<std::string>::iterator it = value.begin(); it != value.end(); ++it) {
            info.ifAddr = *it;
            info.ifName = CFG_VIRTUAL_IP;
            result.push_back(info);
        }
    }
    return result;
}

/**
 * @brief  : check path valid
 * @return : true : valid path, false : invalid path
 */
bool CfgConfig::CheckPathValid(std::string &path)
{
    if (!path.empty() &&
        path.back() != IdeDaemon::Common::Config::OS_SPLIT) {
        path += IdeDaemon::Common::Config::OS_SPLIT_STR;
    }

    /* check path length */
    if (path.length() == 0 || path.length() > MAX_CFG_PATH_LENGTH) {
        IDE_LOGW("exceed path length(0, 1024]");
        return false;
    }

    /* directory exist /.., ../, /../ */
    if (Adx::FileUtils::CheckNonCrossPath(path) == false) {
        IDE_LOGW("exists cross path");
        return false;
    }

    /* check path exist valid char */
    if (IdeDaemon::Common::Utils::IsValidDirChar(path) == false) {
        return false;
    }

    /* file exist and path is directory */
    if (Adx::FileUtils::IsFileExist(path) == true &&
        Adx::FileUtils::IsDirectory(path) == false) {
        IDE_LOGW("exists not directory");
        return false;
    }

    return true;
}

/**
 * @brief  : check path valid if invalid use default path;
 * @return : valid path
 */
std::string CfgConfig::GetValidPathByKey(const std::string &key, const std::string &defaultPath)
{
    std::string value = defaultPath;
    if (this->parse_ != nullptr && this->parse_->GetValueByKey(key, value)) {
        if (CheckPathValid(value)) {
            return value;
        }
    }

    return defaultPath;
}

/**
 * @brief  : get install mode;
 * @return : install mode.0:default_mode, 1:install for all;
 */
int CfgConfig::GetInstallMode() const
{
    int value = 0;
    if (this->parse_ != nullptr && this->parse_->GetValueByKey(INSTALL_MODE, value)) {
        if (value >= MIN_SOCK_PORT && value <= MAX_SOCK_PORT) {
            return value;
        }
    }

    return DEFAULT_INSTALL_MODE;
}
}
}
}
