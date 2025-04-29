/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : init.cpp
 * Description        : Cpython interface bind.
 * Author             : msprof team
 * Creation Date      : 2024/11/19
 * *****************************************************************************
*/

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <string>
#include "mspti_adapter.h"

#ifdef ASCEND_CI_LIMITED_PY37
#undef PyCFunction_NewEx
#endif

namespace Mspti {
namespace Adapter {
PyObject *MsptiStart(PyObject *self, PyObject *args)
{
    auto ret = MsptiAdapter::GetInstance()->Start();
    return Py_BuildValue("i", ret);
}

PyObject *MsptiStop(PyObject *self, PyObject *args)
{
    auto ret = MsptiAdapter::GetInstance()->Stop();
    return Py_BuildValue("i", ret);
}

PyObject *MsptiFlushAll(PyObject *self, PyObject *args)
{
    auto ret = MsptiAdapter::GetInstance()->FlushAll();
    return Py_BuildValue("i", ret);
}

PyObject *MsptiFlushPeriod(PyObject *self, PyObject *args)
{
    uint32_t time = 0;
    if (!PyArg_ParseTuple(args, "I", &time)) {
        PyErr_SetString(PyExc_TypeError, "FlushPeriod time args parse failed!");
        return nullptr;
    }
    auto ret = MsptiAdapter::GetInstance()->FlushPeriod(time);
    return Py_BuildValue("i", ret);
}

PyObject *MsptiSetBufferSize(PyObject *self, PyObject *args)
{
    uint32_t size = 0;
    if (!PyArg_ParseTuple(args, "I", &size)) {
        PyErr_SetString(PyExc_TypeError, "SetBufferSize size args parse failed!");
        return nullptr;
    }
    auto ret = MsptiAdapter::GetInstance()->SetBufferSize(size);
    return Py_BuildValue("i", ret);
}

namespace Mstx {
PyObject *RegisterCB(PyObject *self, PyObject *args)
{
    PyObject *callback = nullptr;
    if (!PyArg_ParseTuple(args, "O", &callback)) {
        PyErr_SetString(PyExc_TypeError, "Mstx register callback args parse failed!");
        return nullptr;
    }
    auto ret = MsptiAdapter::GetInstance()->RegisterMstxCallback(callback);
    return Py_BuildValue("i", ret);
}

PyObject *UnregisterCB(PyObject *self, PyObject *args)
{
    auto ret = MsptiAdapter::GetInstance()->UnregisterMstxCallback();
    return Py_BuildValue("i", ret);
}

PyMethodDef *GetMstxMethods()
{
    static PyMethodDef methodMstx[] = {
        {"registerCB", RegisterCB, METH_VARARGS, ""},
        {"unregisterCB", UnregisterCB, METH_NOARGS, ""},
        {nullptr, nullptr, METH_VARARGS, ""}
    };
    return methodMstx;
}
} // Mstx

namespace Kernel {
PyObject *RegisterCB(PyObject *self, PyObject *args)
{
    PyObject *callback = nullptr;
    if (!PyArg_ParseTuple(args, "O", &callback)) {
        PyErr_SetString(PyExc_TypeError, "Kernel register callback args parse failed!");
        return nullptr;
    }
    auto ret = MsptiAdapter::GetInstance()->RegisterKernelCallback(callback);
    return Py_BuildValue("i", ret);
}

PyObject *UnregisterCB(PyObject *self, PyObject *args)
{
    auto ret = MsptiAdapter::GetInstance()->UnregisterKernelCallback();
    return Py_BuildValue("i", ret);
}

PyMethodDef *GetKernelMethods()
{
    static PyMethodDef methodMstx[] = {
        {"registerCB", RegisterCB, METH_VARARGS, ""},
        {"unregisterCB", UnregisterCB, METH_NOARGS, ""},
        {nullptr, nullptr, METH_VARARGS, ""}
    };
    return methodMstx;
}
} // Kernel

namespace Hccl {
PyObject *RegisterCB(PyObject *self, PyObject *args)
{
    PyObject *callback = nullptr;
    if (!PyArg_ParseTuple(args, "O", &callback)) {
        PyErr_SetString(PyExc_TypeError, "hccl register callback args parse failed!");
        return nullptr;
    }
    auto ret = MsptiAdapter::GetInstance()->RegisterHcclCallback(callback);
    return Py_BuildValue("i", ret);
}

PyObject *UnregisterCB(PyObject *self, PyObject *args)
{
    auto ret = MsptiAdapter::GetInstance()->UnregisterHcclCallback();
    return Py_BuildValue("i", ret);
}

PyMethodDef *GetHcclMethods()
{
    static PyMethodDef methodHccl[] = {
        {"registerCB", RegisterCB, METH_VARARGS, ""},
        {"unregisterCB", UnregisterCB, METH_NOARGS, ""},
        {nullptr, nullptr, METH_VARARGS, ""}
    };
    return methodHccl;
}
} // Hccl
} // Adapter
} // Mspti

static PyMethodDef g_moduleMethods[] = {
    {"start", Mspti::Adapter::MsptiStart, METH_NOARGS, ""},
    {"stop", Mspti::Adapter::MsptiStop, METH_NOARGS, ""},
    {"flush_all", Mspti::Adapter::MsptiFlushAll, METH_NOARGS, ""},
    {"flush_period", Mspti::Adapter::MsptiFlushPeriod, METH_VARARGS, ""},
    {"set_buffer_size", Mspti::Adapter::MsptiSetBufferSize, METH_VARARGS, ""},
    {nullptr, nullptr, METH_VARARGS, ""}
};

static PyModuleDef g_libMethods = {
    PyModuleDef_HEAD_INIT, "mspti_C",
    "",
    -1,
    g_moduleMethods,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

static void AddSubModule(PyObject *root, const char *name, PyMethodDef *methods)
{
    PyObject *d = PyModule_GetDict(root);
    std::string subModuleName = name;
    std::string moduleName = "mspti_C." + subModuleName;
    PyObject *subModule = PyDict_GetItemString(d, name);
    if (subModule == nullptr) {
        subModule = PyImport_AddModule(moduleName.c_str());
        PyDict_SetItemString(d, name, subModule);
        Py_XDECREF(subModule);
    }
    // populate module's dict
    d = PyModule_GetDict(subModule);
    for (PyMethodDef *m = methods; m->ml_name != nullptr; ++m) {
        PyObject *methodObj = PyCFunction_NewEx(m, nullptr, nullptr);
        PyDict_SetItemString(d, m->ml_name, methodObj);
        Py_XDECREF(methodObj);
    }
}

#if defined(__cplusplus)
extern "C" {
#endif

PyMODINIT_FUNC PyInit_mspti_C()
{
    PyObject *m = PyModule_Create(&g_libMethods);
    AddSubModule(m, "mstx", Mspti::Adapter::Mstx::GetMstxMethods());
    AddSubModule(m, "kernel", Mspti::Adapter::Kernel::GetKernelMethods());
    AddSubModule(m, "hccl", Mspti::Adapter::Hccl::GetHcclMethods());
    return m;
}

__attribute__ ((visibility("default"))) PyObject* PyInit_mspti_C();

#if defined(__cplusplus)
}
#endif
