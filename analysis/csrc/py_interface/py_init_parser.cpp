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
#include "analysis/csrc/worker/kernel_parser_worker.h"
#include "analysis/csrc/utils/file.h"
#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/dfx/error_code.h"

namespace Analysis {
namespace PyInterface {
using KernelParserWorker = Analysis::Worker::KernelParserWorker;
using namespace Analysis::Utils;
PyMethodDef g_methodTestSchedule[] = {
    {"dump_cann_trace", WrapDumpCANNTrace, METH_VARARGS, ""},
    {NULL, NULL, METH_VARARGS, ""}
};

PyMethodDef *GetParserMethods()
{
    return g_methodTestSchedule;
};

PyObject *WrapDumpCANNTrace(PyObject *self, PyObject *args)
{
    const char *parseFilePath = NULL;
    if (!PyArg_ParseTuple(args, "s", &parseFilePath)) {
        PyErr_SetString(PyExc_TypeError, "parser.dump_cann_trace args parse failed!");
        return NULL;
    }
    if (!File::CheckDir(parseFilePath)) {
        ERROR("Parse failed or dump failed.");
        return Py_BuildValue("i", ANALYSIS_ERROR);
    }
    KernelParserWorker parserWorker(parseFilePath);
    auto res = parserWorker.Run();
    return Py_BuildValue("i", res);
}
}
}