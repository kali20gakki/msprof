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
namespace Analysis {
namespace PyInterface {
using KernelParserWorker = Analysis::Worker::KernelParserWorker;
PyMethodDef g_methodTestSchedule[] = {
    {"dump_cann_trace", WrapDumpCANNTrace, METH_VARARGS, ""},
    {NULL, NULL}
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
    KernelParserWorker parserWorker(parseFilePath);
    auto res = parserWorker.Run();
    return Py_BuildValue("i", res);
}
}
}