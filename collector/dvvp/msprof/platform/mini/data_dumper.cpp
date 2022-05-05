/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: dump stream file
 * Author: yc
 * Create: 2019-12-20
 */
#include "data_dumper.h"
#include "receive_data.h"

namespace Msprof {
namespace Engine {
/* *
 * @brief DataDumper: the construct function
 * @param [in] module: the name of the plugin
 */
DataDumper::DataDumper() : ReceiveData()
{
}

DataDumper::~DataDumper()
{
}
}
}