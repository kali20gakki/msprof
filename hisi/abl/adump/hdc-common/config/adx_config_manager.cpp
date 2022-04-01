/**
 * @file adx_config_manager.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "adx_config_manager.h"
#include "common/utils.h"
#include "ide_common_util.h"
namespace Adx {
namespace Manager {
namespace Config {
AdxConfigManager::AdxConfigManager()
    : defaultTime_(true),
      sockSwitch_(true),
      installMode_(Adx::Common::Config::DEFAULT_INSTALL_MODE),
      netPort_(Adx::Common::Config::DEFAULT_HOST_PORT),
      certExprie_(Adx::Common::Config::DEFAULT_CERT_EXPIRE_DAY),
      config_(nullptr)
{
}

AdxConfigManager::~AdxConfigManager()
{
}

/**
 * @berif : init config of cfg file
 * @return : cert cipher value
 */
bool AdxConfigManager::Init()
{
    cfgPath_ = GetCfgPath();
    config_ = std::unique_ptr<CfgConfig>(
        new (std::nothrow)CfgConfig(ConfigParseFactory::MakeConfigParse(
            "cfg", cfgPath_)));
    if (config_ == nullptr) {
        return false;
    }
    UpdateConfig();
    return true;
}

/**
 * @berif : get default time flag
 * @return : true : set default time
 *          false : don't set default time
 */
bool AdxConfigManager::GetDefaultTime() const
{
    return defaultTime_;
}

/**
 * @berif : set default time flag
 * @return : none
 */
void AdxConfigManager::SetDefaultTime(bool value)
{
    defaultTime_ = value;
}

/**
 * @berif : get sock switch flag
 * @return : true : enable socket
 *           false : disable socket
 */
bool AdxConfigManager::GetSockSwitch() const
{
#if (defined IDE_DAEMON_HOST)
    return sockSwitch_;
#else
    return false;
#endif
}

/**
 * @berif : set sock switch flag
 * @return : none
 */
void AdxConfigManager::SetSockSwitch(bool value)
{
    sockSwitch_ = value;
}

int AdxConfigManager::GetHostPort() const
{
    return netPort_;
}

void AdxConfigManager::SetHostPort(int value)
{
    IDE_LOGI("set host post : %d", value);
    netPort_ = value;
}

std::string AdxConfigManager::GetTlsOption()
{
    return tlsOption_;
}

void AdxConfigManager::SetTlsOption(const std::string &value)
{
    IDE_LOGI("set tls option : %s", value.c_str());
    tlsOption_ = value;
}

unsigned long AdxConfigManager::GetCertExprie() const
{
    if (config_ == nullptr) {
        return Adx::Common::Config::DEFAULT_CERT_EXPIRE_DAY;
    }

    return config_->GetCertExprie();
}

void AdxConfigManager::SetCertExprie(unsigned long value)
{
    IDE_LOGI("set cert exprie : %lu", value);
    certExprie_ = value;
}

/**
 * @berif : get openssl cert cipher
 * @return : cert cipher value
 */
std::string AdxConfigManager::GetCertCipher()
{
    return certCipher_;
}

/**
 * @berif : get adx workspace path
 * @return : adx workapce path
 */
void AdxConfigManager::SetCertCipher(const std::string &value)
{
    IDE_LOGI("set cert cipher : %s", value.c_str());
    certCipher_ = value;
}

/**
 * @berif : get adx workspace path
 * @return : adx workapce path
 */
std::string AdxConfigManager::GetWorkPath()
{
    return workPath_;
}

void AdxConfigManager::SetWorkPath(const std::string &path)
{
    workPath_ = path;
    if (!path.empty() && path.back() != IdeDaemon::Common::Config::OS_SPLIT) {
        workPath_ += IdeDaemon::Common::Config::OS_SPLIT_STR;
    }

    rworkPath_ = IdeDaemon::Common::Utils::ReplaceWaveToHomeDir(workPath_);
    adtmpPath_ = rworkPath_ + Adx::Common::Config::DEFAULT_ADTMP_PATH;
    if (!CheckAndCreatePath(adtmpPath_)) { // check error change default work path
        workPath_ = Adx::Common::Config::DEFAULT_WORK_PATH;
        rworkPath_ = IdeDaemon::Common::Utils::ReplaceWaveToHomeDir(workPath_);
        adtmpPath_ = rworkPath_ + Adx::Common::Config::DEFAULT_ADTMP_PATH;
        (void)CheckAndCreatePath(adtmpPath_);
    }
#ifndef IDE_DAEMON_CLIENT
    // change workspace to WORKSPACE
    if (mmChdir(adtmpPath_.c_str()) < 0) {
        char errBuf[MAX_ERRSTR_LEN + 1] = {0};
        IDE_LOGE("chdir %s failed, errmsg: %s", path.c_str(),
                 mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));
    }
#endif
    IDE_LOGI("set work path: %s", workPath_.c_str());
    IDE_LOGI("set replace path : %s", rworkPath_.c_str());
    IDE_LOGI("set adtmp path: %s", adtmpPath_.c_str());
}

/**
 * @berif : get adx tmp path (work path + tmp)
 * @return : adx tmp path
 */
std::string AdxConfigManager::GetAdtmpPath()
{
    (void)CheckAndCreatePath(adtmpPath_);
    return adtmpPath_;
}

/**
 * @berif : set adx cert path
 * @return : adx cert path
 */
void AdxConfigManager::SetCertPath(const std::string &path)
{
    IDE_LOGI("set cert path : %s", path.c_str());
    certPath_ = path;
}

/**
 * @berif : get operate log path
 * @return : adx log path
 */
std::string AdxConfigManager::GetLogPath()
{
    (void)CheckAndCreatePath(logPath_);
    return logPath_;
}

/**
 * @berif : set operate log path
 * @return : None
 */
void AdxConfigManager::SetLogPath(const std::string &path)
{
    if (Adx::FileUtils::IsDirExist(path) == false) {
        IDE_LOGW("can't access log path : %s, use default", path.c_str());
        logPath_ = rworkPath_ + Adx::Common::Config::DEFAULT_LOG_PATH;
    } else {
        logPath_ = path;
    }
    IDE_LOGI("set log path : %s", logPath_.c_str());
}

/**
 * @berif : get config path
 * @return : None
 */
std::string AdxConfigManager::GetCfgPath()
{
    cfgPath_ = IdeDaemon::Common::Utils::ReplaceWaveToHomeDir(
        Adx::Common::Config::DEFAULT_CFG_PATH);
#if (defined IDE_DAEMON_HOST) // host find ~ first
    if (CheckVerifyFileExist(cfgPath_) == true) {
        IDE_LOGI("verify path : %s", cfgPath_.c_str());
        return cfgPath_;
    }
#endif

    if (!SearchCurrVerifyPath(cfgPath_)) {
        IDE_LOGW("can't find config file use default %s", cfgPath_.c_str());
    }
    IDE_LOGI("verify path : %s", cfgPath_.c_str());
    return cfgPath_;
}

/**
 * @berif : get self path
 * @return : current exe path
 */
std::string AdxConfigManager::GetSelfPath()
{
    IdeString selfBin = "/proc/self/exe";
    uint32_t pathSize = MMPA_MAX_PATH + 1;
    IdeStringBuffer curPath = reinterpret_cast<IdeStringBuffer>(IdeXmalloc(pathSize));
    IDE_CTRL_VALUE_FAILED(curPath != nullptr, return "", "malloc failed");
    int len = readlink(selfBin, curPath, MMPA_MAX_PATH); // read self path of store
    if (len < 0 || len > MMPA_MAX_PATH) {
        IDE_LOGW("Can't Get self bin directory");
        IDE_XFREE_AND_SET_NULL(curPath);
        return "";
    }

    curPath[len] = '\0'; // add string end char
    selfPath_ = curPath;
    IDE_XFREE_AND_SET_NULL(curPath);
    return selfPath_;
}

/**
 * @berif : get kmc secu value
 * @return : kmc secu value
 */
std::string AdxConfigManager::GetSecuValue()
{
    return secuValue_;
}

/**
 * @berif : set kmc secu value
 * @return : none
 */
void AdxConfigManager::SetSecuValue(const std::string &value)
{
    secuValue_ = value;
}

/**
 * @berif : get kmc store value
 * @return : kmc store value
 */
std::string AdxConfigManager::GetStoreValue()
{
    return storeValue_;
}

/**
 * @berif : set kmc store value
 * @return : kmc store value
 */
void AdxConfigManager::SetStoreValue(const std::string &value)
{
    storeValue_ = value;
}

/**
 * @berif : get host ip info
 * @return : ip info
 */
std::vector<NetAdapterIpAddr> AdxConfigManager::GetHostIpInfo()
{
    return netIfInfo_;
}

/**
 * @berif : set host ip info
 * @return : ip info
 */
void AdxConfigManager::SetHostIpInfo(const std::vector<NetAdapterIpAddr> &info)
{
    netIfInfo_ = info;
}

/**
 * @berif : update all config
 * @return : none
 */
void AdxConfigManager::UpdateConfig()
{
    if (config_ == nullptr) {
        return;
    }

    SetCertPath(cfgPath_);
    SetWorkPath(config_->GetWorkPath());
    SetCertExprie(config_->GetCertExprie());
    SetCertCipher(config_->GetCertCipher());
    SetTlsOption(config_->GetTlsOption());
    SetSecuValue(config_->GetSecuValue());
    SetStoreValue(config_->GetStoreValue());
    SetLogPath(config_->GetLogPath());
    SetDefaultTime(config_->GetDefaultTime());
#ifndef IDE_DAEMON_CLIENT
    SetSockSwitch(this->config_->GetSockSwitch());
    SetHostIpInfo(this->config_->GetHostIpInfo());
    SetHostPort(this->config_->GetHostPort());
    SetInstallMode(this->config_->GetInstallMode());
#endif
}

/**
 * @berif : check path , not exist create path
 * @return :
 *         true : check path success
 *         false : check path failed
 */
bool AdxConfigManager::CheckAndCreatePath(const std::string &value) const
{
    if (!Adx::FileUtils::IsFileExist(value)) {
        if (Adx::FileUtils::CreateDir(value) != IDE_DAEMON_NONE_ERROR) {
            return false;
        }
    }

    if (mmAccess2(value.c_str(), R_OK | W_OK | X_OK) != EN_OK) {
        char errBuf[MAX_ERRSTR_LEN + 1] = {0};
        IDE_LOGW("check path rwx permission : %s", mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));
        return false;
    }
    return true;
}

/**
* @brief check current path of verify files
* @param [in] path : verify files path
*
* @return
*         true : find the verify path
*        flase : can't find the verify path
*/
bool AdxConfigManager::CheckVerifyFileExist(const std::string &verPath) const
{
    // check config file is exist or not
    std::string checkPath = verPath + IdeDaemon::Common::Config::IDE_CFG_VALUE;
    FILE_CHECK_EXIST(checkPath, return false);

    // check verify ca config is exist or not
    checkPath = verPath + IdeDaemon::Common::Config::SSL_CA_FILE;
    FILE_CHECK_EXIST(checkPath, return false);
    return true;
}

/**
* @brief search current path of verify files
* @param [out] path : verify files path
*
* @return
*         true : find the verify path
*        flase : can't find the verify path
*/
bool AdxConfigManager::SearchCurrVerifyPath(std::string &path)
{
    const std::string kitPath = "/toolkit";
    const std::string kitConfPath = "/tools/ide_daemon/conf/";
    std::string verPath = GetSelfPath();
    size_t pos = verPath.rfind(IdeDaemon::Common::Config::OS_SPLIT_STR);
    // get verPath with os split
    verPath = (pos == std::string::npos) ? verPath : verPath.substr(0, pos + 1);
    bool ret = CheckVerifyFileExist(verPath); // check current path
    if (!ret) {
        pos = verPath.rfind(kitPath);
        if (pos == std::string::npos) {
            IDE_LOGW("can't find toolkit directory");
            return false;
        }

        verPath = verPath.substr(0, pos);
        verPath += kitPath + kitConfPath;
        ret = CheckVerifyFileExist(verPath);
    }

    path = verPath;
    return ret;
}

/**
 * @berif : get install mode
 * @return : kmc store value
 */
int AdxConfigManager::GetInstallMode() const
{
    return installMode_;
}

/**
 * @berif : set install mode value
 */
void AdxConfigManager::SetInstallMode(const int &mode)
{
    installMode_ = mode;
}
}
}
}
