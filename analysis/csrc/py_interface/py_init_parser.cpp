/* ******************************************************************************
           版权所有 (c) 华为技术有限公司 2023-2023
           Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : py_init_parser.cpp
 * Description        : 注册dump_cann_trace接口
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#include "analysis/csrc/py_interface/py_init_parser.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/worker/kernel_parser_worker.h"
#include "analysis/csrc/utils/file.h"
#include "analysis/csrc/viewer/database/finals/unified_db_manager.h"
#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/dfx/error_code.h"

namespace Analysis {
namespace PyInterface {
using KernelParserWorker = Analysis::Worker::KernelParserWorker;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
PyMethodDef g_methodTestSchedule[] = {
    {"dump_cann_trace", WrapDumpCANNTrace, METH_VARARGS, ""},
    {"dump_device_data", WrapDumpDeviceData, METH_VARARGS, ""},
    {"export_unified_db", WrapExportUnifiedDB, METH_VARARGS, ""},
    {NULL, NULL, METH_VARARGS, ""}
};

PyMethodDef *GetParserMethods()
{
    return g_methodTestSchedule;
};

PyObject *WrapDumpCANNTrace(PyObject *self, PyObject *args)
{
    // parseFilePath为PROF*目录下面的host目录
    const char *parseFilePath = NULL;
    if (!PyArg_ParseTuple(args, "s", &parseFilePath)) {
        PyErr_SetString(PyExc_TypeError, "parser.dump_cann_trace args parse failed!");
        return NULL;
    }
    if (!File::CheckDir(parseFilePath)) {
        PyErr_SetString(PyExc_TypeError, "parser.dump_cann_trace path is invalid!");
        return NULL;
    }
    Log::GetInstance().Init(Utils::File::PathJoin({parseFilePath, "..", "mindstudio_profiler_log"}));
    if (!File::CheckDir(parseFilePath)) {
        ERROR("Parse failed or dump failed.");
        return Py_BuildValue("i", ANALYSIS_ERROR);
    }
    KernelParserWorker parserWorker(parseFilePath);
    auto res = parserWorker.Run();
    return Py_BuildValue("i", res);
}

PyObject *WrapDumpDeviceData(PyObject *self, PyObject *args)
{
    // parseFilePath为PROF*目录
    const char *parseFilePath = NULL;
    if (!PyArg_ParseTuple(args, "s", &parseFilePath)) {
        PyErr_SetString(PyExc_TypeError, "parser.dump_device_data args parse failed!");
        return NULL;
    }
    if (!File::CheckDir(parseFilePath)) {
        PyErr_SetString(PyExc_TypeError, "parser.dump_device_data path is invalid!");
        return NULL;
    }
    Log::GetInstance().Init(Utils::File::PathJoin({parseFilePath, "mindstudio_profiler_log"}));
    if (!File::CheckDir(parseFilePath)) {
        ERROR("Parse failed or dump failed.");
        return Py_BuildValue("i", ANALYSIS_ERROR);
    }
    const char *stopAt = "";
    DeviceContextEntry(parseFilePath, stopAt);
    return Py_BuildValue("i", ANALYSIS_OK);
}

PyObject *WrapExportUnifiedDB(PyObject *self, PyObject *args)
{
    // parseFilePath为PROF父目录或者PROF*目录
    const char *parseFilePath = NULL;
    if (!PyArg_ParseTuple(args, "s", &parseFilePath)) {
        PyErr_SetString(PyExc_TypeError, "parser.export_unified_db args parse failed!");
        return NULL;
    }
    if (!File::CheckDir(parseFilePath)) {
        PyErr_SetString(PyExc_TypeError, "parser.export_unified_db path is invalid!");
        return NULL;
    }
    Log::GetInstance().Init(parseFilePath);
    auto unifiedDbManager = Analysis::Viewer::Database::UnifiedDBManager(parseFilePath);
    if (!unifiedDbManager.Init()) {
        ERROR("UnifiedDB init failed.");
        return Py_BuildValue("i", ANALYSIS_ERROR);
    }
    if (!unifiedDbManager.Run()) {
        ERROR("UnifiedDB run failed.");
        return Py_BuildValue("i", ANALYSIS_ERROR);
    }

    return Py_BuildValue("i", ANALYSIS_OK);
}
}
}