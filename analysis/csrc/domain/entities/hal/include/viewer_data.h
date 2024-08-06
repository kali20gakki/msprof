/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : viewer_data.h
 * Description        : viewer_data结构
 * Author             : msprof team
 * Creation Date      : 2024/07/19
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_VIEWER_DATA_H
#define ANALYSIS_DOMAIN_VIEWER_DATA_H

#include <vector>

namespace Analysis {
namespace Domain {
template<typename Tp, typename KeyStruct>
struct ViewerData {
    std::vector<Tp> data;
};
}
}
#endif // ANALYSIS_DOMAIN_VIEWER_DATA_H
