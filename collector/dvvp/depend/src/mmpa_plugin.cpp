#include "mmpa_plugin.h"

INT32 MmpaPlugin::MsprofMmOpen2(const CHAR *pathName, INT32 flags, MODE mode)
{
    if (!pluginManager_.HasLoad()) {
        Status ret = pluginManager_.OpenPlugin(soName_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_HALHDCRECV_T func;
    ret = pluginManager_.GetFunction<const CHAR *, INT32, MODE>("mmOpen2", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(pathName, flags, mode);
}

mmSsize_t MmpaPlugin::MsprofMmRead(INT32 fd, VOID *buf, UINT32 bufLen)
{
    if (!pluginManager_.HasLoad()) {
        Status ret = pluginManager_.OpenPlugin(soName_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_HALHDCRECV_T func;
    ret = pluginManager_.GetFunction<INT32, VOID *, UINT32>("mmRead", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(fd, buf, bufLen);
}

INT32 MmpaPlugin::MsprofMmClose(INT32 fd)
{
    if (!pluginManager_.HasLoad()) {
        Status ret = pluginManager_.OpenPlugin(soName_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_HALHDCRECV_T func;
    ret = pluginManager_.GetFunction<INT32>("mmClose", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(fd);
}

CHAR *MmpaPlugin::MsprofMmGetErrorFormatMessage(mmErrorMsg errnum, CHAR *buf, mmSize size)
{
    if (!pluginManager_.HasLoad()) {
        Status ret = pluginManager_.OpenPlugin(soName_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_HALHDCRECV_T func;
    ret = pluginManager_.GetFunction<mmErrorMsg, CHAR *, mmSize>("mmGetErrorFormatMessage", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(errnum, buf, size);
}

INT32 MmpaPlugin::MsprofMmGetErrorCode()
{
    if (!pluginManager_.HasLoad()) {
        Status ret = pluginManager_.OpenPlugin(soName_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_HALHDCRECV_T func;
    ret = pluginManager_.GetFunction<>("mmGetErrorCode", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func();
}

INT32 MmpaPlugin::MsprofMmAccess2(const CHAR *pathName, INT32 mode)
{
    if (!pluginManager_.HasLoad()) {
        Status ret = pluginManager_.OpenPlugin(soName_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_HALHDCRECV_T func;
    ret = pluginManager_.GetFunction<const CHAR *, INT32>("mmAccess2", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(pathName, mode);
}

INT32 MmpaPlugin::MsprofMmGetOptLong(INT32 argc,
                   CHAR *const *argv,
                   const CHAR *opts,
                   const mmStructOption *longOpts,
                   INT32 *longIndex)
{
    if (!pluginManager_.HasLoad()) {
        Status ret = pluginManager_.OpenPlugin(soName_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_HALHDCRECV_T func;
    ret = pluginManager_.GetFunction<INT32, CHAR *const *, const CHAR *,
                                    const mmStructOption *, INT32 *>("mmGetOptLong", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(argc, argv, opts, longOpts, longIndex);
}                   

INT32 MmpaPlugin::MsprofMmGetOptInd()
{
    if (!pluginManager_.HasLoad()) {
        Status ret = pluginManager_.OpenPlugin(soName_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_HALHDCRECV_T func;
    ret = pluginManager_.GetFunction<>("mmGetOptInd", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func();
}

CHAR *MmpaPlugin::MsprofMmGetOptArg()
{
    if (!pluginManager_.HasLoad()) {
        Status ret = pluginManager_.OpenPlugin(soName_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_HALHDCRECV_T func;
    ret = pluginManager_.GetFunction<>("mmGetOptArg", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func();
}

INT32 MmpaPlugin::MsprofMmSleep(UINT32 milliSecond)
{
    if (!pluginManager_.HasLoad()) {
        Status ret = pluginManager_.OpenPlugin(soName_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_HALHDCRECV_T func;
    ret = pluginManager_.GetFunction<UINT32>("mmSleep", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(milliSecond);
}

INT32 MmpaPlugin::MsprofMmUnlink(const CHAR *filename)
{
    if (!pluginManager_.HasLoad()) {
        Status ret = pluginManager_.OpenPlugin(soName_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_HALHDCRECV_T func;
    ret = pluginManager_.GetFunction<const CHAR *>("mmUnlink", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func(filename);
}

    using MSPROF_MMGETTID_T = INT32 (*)();
INT32 MmpaPlugin::MsprofMmGetTid()
{
    if (!pluginManager_.HasLoad()) {
        Status ret = pluginManager_.OpenPlugin(soName_);
        if (ret != PLUGIN_LOAD_SUCCESS) {
            return -1;
        }
    }
    MSPROF_HALHDCRECV_T func;
    ret = pluginManager_.GetFunction<>("mmGetTid", func);
    if (ret != PLUGIN_LOAD_SUCCESS) {
        return -1;
    }
    return func();
}

