#include "prof_api.h"

int32_t profRegReporterCallback(ProfReportHandle reporter)
{
    return 0;
}

int32_t profRegCtrlCallback(ProfCtrlHandle hanle)
{
    return 0;
}

int32_t profRegDeviceStateCallback(ProfSetDeviceHandle hanle)
{
    return 0;
}

int32_t profGetDeviceIdByGeModelIdx(const uint32_t modelIdx, uint32_t *deviceId)
{
    return 0;
}

int32_t profSetProfCommand(PROFAPI_PROF_COMMAND_PTR command, uint32_t len)
{
    return 0;
}

int32_t profSetStepInfo(const uint64_t indexId, const uint16_t tagId, void* const stream)
{
    return 0;
}

int32_t MsprofInit(uint32_t moduleId, void *data, uint32_t dataLen)
{
    return 0;
}

int32_t MsprofRegisterCallback(uint32_t moduleId, ProfCommandHandle handle)
{
    return 0;
}

int32_t MsprofReportData(uint32_t moduleId, uint32_t type, void* data, uint32_t len)
{
    return 0;
}

int32_t MsprofSetDeviceIdByGeModelIdx(const uint32_t geModelIdx, const uint32_t deviceId)
{
    return 0;
}

int32_t MsprofUnsetDeviceIdByGeModelIdx(const uint32_t geModelIdx, const uint32_t deviceId)
{
    return 0;
}

int32_t MsprofNotifySetDevice(uint32_t chipId, uint32_t deviceId, bool isOpen)
{
    return 0;
}

int32_t MsprofFinalize()
{
    return 0;
}

/* ACL API */
int32_t profAclInit(uint32_t type, const char *profilerPath, uint32_t len)
{
    return 0;
}

int32_t profAclStart(uint32_t type, PROFAPI_CONFIG_CONST_PTR profilerConfig)
{
    return 0;
}

int32_t profAclStop(uint32_t type, PROFAPI_CONFIG_CONST_PTR profilerConfig)
{
    return 0;
}

int32_t profAclFinalize(uint32_t type)
{
    return 0;
}

int32_t profAclSubscribe(uint32_t type, uint32_t modelId, PROFAPI_SUBSCRIBECONFIG_CONST_PTR config)
{
    return 0;
}

int32_t profAclUnSubscribe(uint32_t type, uint32_t modelId)
{
    return 0;
}

int32_t profAclDrvGetDevNum()
{
    return 0;
}

int64_t profAclGetOpTime(uint32_t type, const void *opInfo, size_t opInfoLen, uint32_t index)
{
    return 0;
}

int32_t profAclGetId(uint32_t type, const void *opInfo, size_t opInfoLen, uint32_t index)
{
    return 0;
}

int32_t profAclGetOpVal(uint32_t type, const void *opInfo, size_t opInfoLen,
                                      uint32_t index, void *data, size_t len)
{
    return 0;
}

uint64_t ProfGetOpExecutionTime(const void *data, uint32_t len, uint32_t index)
{
    return 0;
}

const char *profAclGetOpAttriVal(uint32_t type, const void *opInfo, size_t opInfoLen,
                                               uint32_t index, uint32_t attri)
{
    return nullptr;
}