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
import os

from common_func.singleton import singleton
from common_func.path_manager import PathManager
from common_func.db_name_constant import DBNameConstant
from msmodel.add_info.runtime_op_info_model import RuntimeOpInfoViewModel
from profiling_bean.db_dto.runtime_op_info_dto import RuntimeOpInfoDto


@singleton
class RTAddInfoCenter:

    def __init__(self: any, project_path: str) -> None:
        self._op_info_dict = {}
        if os.path.exists(PathManager.get_db_path(project_path, DBNameConstant.DB_RTS_TRACK)):
            self.load_op_info_data(project_path)

    def load_op_info_data(self: any, project_path: str) -> None:
        """
        load runtime op info data
        """
        with RuntimeOpInfoViewModel(project_path) as _model:
            self._op_info_dict = _model.get_runtime_op_info_data()

    def get_op_info_by_id(self: any, device_id: int, stream_id: int, task_id: int) -> RuntimeOpInfoDto:
        """
        get type hash dict data
        后续需要增加batchId做唯一id关联，batchId应通过capture stream info数据获取
        """
        return self._op_info_dict.get((device_id, stream_id, task_id), RuntimeOpInfoDto())

