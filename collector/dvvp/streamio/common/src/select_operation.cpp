/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: yutianqi
 * Create: 2018-06-13
 */
#include "select_operation.h"
#include "mmpa_api.h"

namespace analysis {
namespace dvvp {
namespace streamio {
namespace common {
SelectOperation::SelectOperation()
    : maxFd_(0)
{
    FD_ZERO(&readfd_);
}

SelectOperation::~SelectOperation()
{
}

void SelectOperation::SelectAdd(mmSockHandle fd)
{
    if (fd < 0) {
        return;
    }

    if (maxFd_ < fd) {
        maxFd_ = fd;
    }

    FD_SET(fd, &readfd_);
}

void SelectOperation::SelectDel(mmSockHandle fd)
{
    if (fd < 0) {
        return;
    }

    FD_CLR(fd, &readfd_);
}

bool SelectOperation::SelectIsSet(mmSockHandle fd)
{
    if (fd < 0) {
        return false;
    }

    if (FD_ISSET(fd, &readfd_)) {
        return true;
    }

    return false;
}

void SelectOperation::SelectClear()
{
    FD_ZERO(&readfd_);
}
}  // namespace common
}  // namespace streamio
}  // namespace dvvp
}  // namespace analysis
