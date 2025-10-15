/* ******************************************************************************
           版权所有 (c) 华为技术有限公司 2023-2023
           Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : py_init_dump_cann_trace.h
 * Description        : 注册python接口
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_PY_INTERFACE_PY_INIT_PARSER_H
#define ANALYSIS_PY_INTERFACE_PY_INIT_PARSER_H

#include <Python.h>
namespace Analysis {
namespace Interface {
// 该方法供注册python接口使用，用于收集PyMethodDef供注册时挂载module
// 挂载接口格式如下： Get{模块名}Methods
PyMethodDef *GetParserMethods();

// 解析落盘CANN数据功能的外层包装，解析python侧传入的路径后调用KernelParserWorker启动解析流程，获取返回状态码后返回python侧
PyObject *WrapDumpCANNTrace(PyObject *self, PyObject *args);
// 解析Device侧数据功能的外层包装，解析python侧传入的路径后调用DeviceContextEntry启动解析流程，获取返回状态码后返回python侧
PyObject *WrapDumpDeviceData(PyObject *self, PyObject *args);
// 统一db入口功能的外层包装，解析python侧传入的路径后调用unifiedDbManager启动导出流程，获取返回状态码后返回python侧
PyObject *WrapExportUnifiedDB(PyObject *self, PyObject *args);
// timeline导出入口的外层包装，解析Python侧传入路径后调用exportManager启动导出流程，获取返回状态码后返回python侧
PyObject *WrapExportTimeline(PyObject *self, PyObject *args);
// op_summary导出入口的外层包装，解析Python侧传入路径后调用exportSummary启动导出流程，获取返回状态码后返回Python侧
PyObject *WrapExportSummary(PyObject *self, PyObject *args);
} // Interface
} // Analyzer

#endif // ANALYSIS_PY_INTERFACE_PY_INIT_PARSER_H