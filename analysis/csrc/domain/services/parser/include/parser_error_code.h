/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : parser_error_code.h
 * Description        : 解析二进制数据错误码
 * Author             : msprof team
 * Creation Date      : 2024/4/22
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_PARSER_ERROR_CODE_H
#define MSPROF_ANALYSIS_PARSER_ERROR_CODE_H

#include "analysis/csrc/dfx/error_code.h"

namespace Analysis {
enum {
    PARSER_GET_FILE_ERROR           = ERROR_NO(SERVICE_ID_PARSER, 0, 0x1),
    PARSER_NEW_BINARY_DATA_ERROR    = ERROR_NO(SERVICE_ID_PARSER, 0, 0x2),
    PARSER_READ_DATA_ERROR          = ERROR_NO(SERVICE_ID_PARSER, 0, 0x3),
    PARSER_PARSE_DATA_ERROR         = ERROR_NO(SERVICE_ID_PARSER, 0, 0x4),
    PARSER_FOPEN_ERROR              = ERROR_NO(SERVICE_ID_PARSER, 0, 0x5),
    PARSER_FREAD_ERROR              = ERROR_NO(SERVICE_ID_PARSER, 0, 0x6),

    PARSER_TS_TRACK_NO_NEED_ITEM    = ERROR_NO(SERVICE_ID_PARSER, 0, 0x7),
};
}
#endif // MSPROF_ANALYSIS_PARSER_ERROR_CODE_H
