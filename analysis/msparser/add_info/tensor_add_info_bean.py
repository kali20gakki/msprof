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

from common_func.ms_constant.level_type_constant import LevelDataType
from profiling_bean.ge.ge_tensor_base_bean import GeTensorBaseBean


class TensorAddInfoBean(GeTensorBaseBean):
    """
    ge tensor info bean
    """
    TENSOR_LEN = 11
    TENSOR_PER_LEN = 8

    def __init__(self: any, *args) -> None:
        super().__init__()
        data = args[0]
        self._level = data[1]
        self._struct_type = data[2]
        self._thread_id = data[3]
        self._data_len = data[4]
        self._timestamp = data[5]
        self._node_id = data[6]
        self._tensor_num = data[7]
        _tensor_datas = []
        for tensor_index in range(0, self._tensor_num):
            _tensor_datas.append(
                list(data[self.TENSOR_LEN * tensor_index + self.TENSOR_PER_LEN:
                                       self.TENSOR_LEN * tensor_index + (self.TENSOR_LEN + self.TENSOR_PER_LEN)]))
        for _tensor_data in _tensor_datas:
            # 0 input 1 output
            if _tensor_data[0] == 0:
                self._input_format.append(_tensor_data[1])
                self._input_data_type.append(_tensor_data[2])
                self._input_shape.append(_tensor_data[3:])
            if _tensor_data[0] == 1:
                self._output_format.append(_tensor_data[1])
                self._output_data_type.append(_tensor_data[2])
                self._output_shape.append(_tensor_data[3:])

    @property
    def level(self: any) -> str:
        """
        level
        """
        return LevelDataType(self._level).name.lower()

    @property
    def struct_type(self: any) -> str:
        """
        type
        """
        return str(self._struct_type)

    @property
    def thread_id(self: any) -> int:
        """
        thread id
        """
        return self._thread_id

    @property
    def data_len(self: any) -> int:
        """
        data length
        """
        return self._data_len

    @property
    def timestamp(self: any) -> int:
        """
        timestamp
        """
        return self._timestamp

    @property
    def tensor_num(self: any) -> int:
        """
        for tensor num
        """
        return self._tensor_num

    @property
    def node_id(self: any) -> str:
        """
        for node id
        """
        return str(self._node_id)
