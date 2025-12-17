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
from common_func.db_name_constant import DBNameConstant
from profiling_bean.struct_info.block_dim_bean import BlockDimBean


class BlockDimReader:
    """
    class for the real block dim under the scene of tiling-down
    """

    def __init__(self: any) -> None:
        self._data = []
        self._table_name = DBNameConstant.TABLE_BLOCK_DIM

    @property
    def data(self: any) -> list:
        """
        list of block dim data from binary
        """
        return self._data

    @property
    def table_name(self):
        """
        the table name for the block dim
        """
        return self._table_name

    def read_binary_data(self: any, bean_data: any) -> None:
        """
        read block dim binary data and store them into list
        :param bean_data: binary data
        :return: None
        """
        block_dim_bean = BlockDimBean.decode(bean_data)
        if block_dim_bean:
            self._data.append(
                (block_dim_bean.timestamp, block_dim_bean.stream_id,
                 block_dim_bean.task_id, block_dim_bean.block_dim))
