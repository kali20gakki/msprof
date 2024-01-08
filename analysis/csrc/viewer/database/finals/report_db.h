/* ******************************************************************************
版权所有 (c) 华为技术有限公司 2023-2023
Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : report_db.h
 * Description        : report_db 定义reportDB中的表结构
 * Author             : msprof team
 * Creation Date      : 2023/12/13
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_REPORT_DB_H
#define ANALYSIS_VIEWER_DATABASE_REPORT_DB_H

#include "analysis/csrc/viewer/database/database.h"


namespace Analysis {
namespace Viewer {
namespace Database {

class ReportDB : public Database {
public:
    ReportDB();
};


} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_REPORT_DB_H
