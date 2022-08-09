#include "devdrv_runtime_api_stub.h"
#include "profapi_plugin.h"
#include "slog_plugin.h"
#include "hccl_plugin.h"

/*
函数原型	drvGetPlatformInfo(u32 *info)
函数功能	获取当前平台信息
输入说明
输出说明
		0 : 表示当前在Device侧
		1 : 表示当前在Host侧
返回值说明	见drvError_t定义
使用说明
注意事项
*/
drvError_t drvGetPlatformInfo(uint32_t *info) {
    if (info) {
        *info = 1;
    }
    return DRV_ERROR_NONE;
}

/*
函数原型	drvError_t drvGetDevNum(u32 *num_dev)
函数功能	获取当前设备个数
输入说明
输出说明
返回值说明	见drvError_t定义
使用说明
注意事项
*/
drvError_t drvGetDevNum(uint32_t *num_dev) {
    *num_dev = 1;
    return DRV_ERROR_NONE;
}

/*
函数原型	drvError_t drvGetDevIDs(u32 *devices)
函数功能	获取当前所有的设备ID
输入说明
输出说明
返回值说明	见drvError_t定义
使用说明
注意事项
*/
drvError_t drvGetDevIDs(uint32_t *devices, uint32_t len) {
    devices[0] = 0;
    return DRV_ERROR_NONE;
}

drvError_t halGetDeviceInfo(uint32_t devId, int32_t moduleType, int32_t infoType, int64_t *value) {
    return DRV_ERROR_NONE;
}

/*
函数原型	drvError_t drvDeviceGetPhyIdByIndex(u32 index, u32 *phyId)
函数功能	get phy id by index
输入说明
输出说明
返回值说明	见drvError_t定义
使用说明
注意事项
*/
drvError_t drvDeviceGetPhyIdByIndex(uint32_t index, uint32_t *phyId) {
    *phyId = index;
    return DRV_ERROR_NONE;
}

/*
函数原型	drvError_t drvDeviceGetIndexByPhyId(u32 phyId, u32 *index)
函数功能	get index by phy id
输入说明
输出说明
返回值说明	见drvError_t定义
使用说明
注意事项
*/
drvError_t drvDeviceGetIndexByPhyId(uint32_t phyId, uint32_t *index) {
    *index = phyId;
    return DRV_ERROR_NONE;
}

/*
函数原型	drvError_t drvGetDevIDByLocalDevID(u32 index, u32 *phyId)
函数功能	get host phy id by dev index
输入说明
输出说明
返回值说明	见drvError_t定义
使用说明
注意事项
*/
drvError_t drvGetDevIDByLocalDevID(uint32_t index, uint32_t *phyId) {
    *phyId = index;
    return DRV_ERROR_NONE;
}
#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t (*MsprofReporterCallback)(uint32_t moduleId, uint32_t type, void *data, uint32_t len);
typedef void (*rtDeviceStateCallback)(uint32_t devId, bool isOpenDevice);


int rtGetDeviceIdByGeModelIdx(uint32_t geModelIdx, uint32_t *deviceId)
{
    return 0;
}

int rtProfSetProSwitch(void* data, uint32_t len)
{
    return 0;
}

int rtSetMsprofReporterCallback(MsprofReporterCallback callBack)
{
    return 0;
}

int rtRegDeviceStateCallback(const char *regName, rtDeviceStateCallback callback)
{
    return 0;
}

int dsmi_get_device_info(unsigned int device_id, unsigned int main_cmd, unsigned int sub_cmd,
    void *buf, unsigned int *size) {
    *(int*)buf = 1;
    return 0;
}
#ifdef __cplusplus
}
#endif


namespace Collector {
namespace Dvvp {
namespace Plugin {
bool ProfApiPlugin::IsFuncExist(const std::string &funcName) const
{
    return true;
}

int32_t ProfApiPlugin::MsprofProfRegReporterCallback(ProfReportHandle reporter)
{
    return 0;
}

int32_t ProfApiPlugin::MsprofProfRegCtrlCallback(ProfCtrlHandle handle)
{
    return 0;
}

int32_t ProfApiPlugin::MsprofProfRegDeviceStateCallback(ProfSetDeviceHandle handle)
{
    return 0;
}

int32_t ProfApiPlugin::MsprofProfGetDeviceIdByGeModelIdx(const uint32_t modelIdx, uint32_t *deviceId)
{
    return 0;
}

int32_t ProfApiPlugin::MsprofProfSetProfCommand(PROFAPI_PROF_COMMAND_PTR command, uint32_t len)
{
    return 0;
}

int32_t ProfApiPlugin::MsprofProfSetStepInfo(const uint64_t indexId, const uint16_t tagId, void* const stream)
{
    return 0;
}

// SlogPlugin::~SlogPlugin() {}

// bool SlogPlugin::IsFuncExist(const std::string &funcName) const
// {
//     return true;
// }

// int SlogPlugin::MsprofCheckLogLevelForC(int moduleId, int logLevel)
// {
//     return 0;
// }

int32_t HcclPlugin::MsprofHcomGetRankId(uint32_t *rankId)
{
    if (rankId == nullptr) {
        return -1;
    } else {
        *rankId = 0;
        return 0;
    }
}

int32_t MsprofHcomGetLocalRankId(uint32_t *localRankId)
{
    if (localRankId == nullptr) {
        return -1;
    } else {
        *localRankId = 0;
        return 0;
    }
}
}}}
