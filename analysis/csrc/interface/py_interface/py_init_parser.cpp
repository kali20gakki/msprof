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

#include "analysis/csrc/interface/py_interface/py_init_parser.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/host_worker/kernel_parser_worker.h"
#include "analysis/csrc/viewer/database/finals/unified_db_manager.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/application/include/export_manager.h"
#include "analysis/csrc/application/include/export_summary.h"

namespace Analysis {
namespace Interface {
using KernelParserWorker = Analysis::Domain::KernelParserWorker;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
PyMethodDef g_methodTestSchedule[] = {
    {"dump_cann_trace", WrapDumpCANNTrace, METH_VARARGS, ""},
    {"dump_device_data", WrapDumpDeviceData, METH_VARARGS, ""},
    {"export_unified_db", WrapExportUnifiedDB, METH_VARARGS, ""},
    {"export_timeline", WrapExportTimeline, METH_VARARGS, ""},
    {"export_op_summary", WrapExportOpSummary, METH_VARARGS, ""},
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
    Log::GetInstance().Init(Utils::File::PathJoin({parseFilePath, "mindstudio_profiler_log"}));
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

PyObject *WrapExportTimeline(PyObject *self, PyObject *args)
{
    // parseFilePath为PROF*目录
    const char *parseFilePath = NULL;
    const char *reportJsonPath = NULL;
    if (!PyArg_ParseTuple(args, "ss", &parseFilePath, &reportJsonPath)) {
        PyErr_SetString(PyExc_TypeError, "parser.export_timeline args parse failed!");
        return NULL;
    }
    if (!File::CheckDir(parseFilePath)) {
        PyErr_SetString(PyExc_TypeError, "parser.export_timeline path is invalid!");
        return NULL;
    }
    if (*reportJsonPath != '\0' && !File::Check(reportJsonPath)) {
        PyErr_SetString(PyExc_TypeError, "parser.export_timeline reports json path is invalid!");
        return NULL;
    }
    Log::GetInstance().Init(Utils::File::PathJoin({parseFilePath, "mindstudio_profiler_log"}));
    auto exportManager = Analysis::Application::ExportManager(parseFilePath, reportJsonPath);
    if (!exportManager.Run()) {
        ERROR("Timeline run failed.");
        return Py_BuildValue("i", ANALYSIS_ERROR);
    }
    return Py_BuildValue("i", ANALYSIS_OK);
}

PyObject *WrapExportOpSummary(PyObject *self, PyObject *args)
{
    // parseFilePath为PROF*目录
    const char *parseFilePath = NULL;
    if (!PyArg_ParseTuple(args, "s", &parseFilePath)) {
        PyErr_SetString(PyExc_TypeError, "parser.export_op_summary args parse failed!");
        return NULL;
    }
    if (!File::CheckDir(parseFilePath)) {
        PyErr_SetString(PyExc_TypeError, "parser.export_op_summary path is invalid!");
        return NULL;
    }
    Log::GetInstance().Init(Utils::File::PathJoin({parseFilePath, "mindstudio_profiler_log"}));
    auto exportSummary = Analysis::Application::ExportSummary(parseFilePath);
    if (!exportSummary.Run()) {
        ERROR("OpSummary run failed.");
        return Py_BuildValue("i", ANALYSIS_ERROR);
    }
    return Py_BuildValue("i", ANALYSIS_OK);
}
}
}