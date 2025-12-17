# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------

from common_func.info_conf_reader import InfoConfReader
from common_func.utils import Utils
from profiling_bean.struct_info.struct_decoder import StructDecoder


class SioDecoder(StructDecoder):
    """
    class used to decode inter soc transmission
    """

    def __init__(self: any, *args: tuple) -> None:
        filed = args[0]
        self._func_type = Utils.get_func_type(filed[0])
        self._sys_cnt = filed[3]
        self._acc_id = filed[5]
        self._req_rx = filed[9]
        self._rsp_rx = filed[10]
        self._snp_rx = filed[11]
        self._dat_rx = filed[12]
        self._req_tx = filed[13]
        self._rsp_tx = filed[14]
        self._snp_tx = filed[15]
        self._dat_tx = filed[16]

    @property
    def func_type(self: any) -> str:
        """
        get func type
        :return: func type
        """
        return self._func_type

    @property
    def data_size(self: any) -> list:
        """
        get sio bandwidth
        :return: (req_rx, rsp_rx, snp_rx, dat_rx, req_tx, rsp_tx, snp_tx, dat_tx)
        """
        return [self._req_rx, self._rsp_rx, self._snp_rx, self._dat_rx,
                self._req_tx, self._rsp_tx, self._snp_tx, self._dat_tx]

    @property
    def acc_id(self: any) -> int:
        """
        get acc id
        :return: acc_id
        """
        return self._acc_id

    @property
    def timestamp(self: any) -> float:
        """
        for sys time
        """
        return InfoConfReader().time_from_syscnt(self._sys_cnt)
