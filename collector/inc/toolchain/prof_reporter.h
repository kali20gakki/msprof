/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#ifndef MSPROF_ENGINE_PROF_REPORTER_H
#define MSPROF_ENGINE_PROF_REPORTER_H

#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
#define MSVP_PROF_API __declspec(dllexport)
#else
#define MSVP_PROF_API __attribute__((visibility("default")))
#endif

#include "prof_callback.h"

/**
 * @file prof_reporter.h
 * @defgroup reporter the reporter group
 * This is the reporter group
 */
namespace Msprof {
namespace Engine {
/**
 * @ingroup reporter
 * @brief class ProfReporter
 *  the ProfReporter class .used to send data to profiling
 */
class MSVP_PROF_API ProfReporter {
public:
    virtual ~ProfReporter() {}

public:
    /**
     * @ingroup reporter
     * @name  : Report
     * @brief : API of libmsprof, report data to libmsprof, it's a non-blocking function \n
                The data will be firstly appended to cache, if the cache is full, data will be ignored
    * @param data [IN] const ReporterData * the data send to libmsporf
    * @retval PROFILING_SUCCESS 0 (success)
    * @retval PROFILING_FAILED -1 (failed)
    *
    * @par depend:
    * @li libmsprof
    * @li prof_reporter.h
    * @since c60
    * @see Flush
    */
    virtual int Report(const ReporterData *data) = 0;

    /**
     * @ingroup reporter
     * @name  : Flush
     * @brief : API of libmsprof, notify libmsprof send data over, it's a blocking function \n
                The all datas of cache will be write to file or send to host
    * @retval PROFILING_SUCCESS 0 (success)
    * @retval PROFILING_FAILED -1 (failed)
    *
    * @par depend:
    * @li libmsprof
    * @li prof_reporter.h
    * @since c60
    * @see ProfMgrStop
    */
    virtual int Flush() = 0;

    virtual uint32_t GetReportDataMaxLen() = 0;
};

}  // namespace Engine
}  // namespace Msprof

#endif  // MSPROF_ENGINE_PROF_REPORTER_H
