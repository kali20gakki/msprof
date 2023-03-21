#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import os

from common_func.common import error
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.path_manager import PathManager
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.torch_npu_dto import TorchNpuDto


class SyncAclNpuModel(ParserModel):
    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_SYNC_ACL_NPU, table_list)

    def flush(self: any, table_name: str, data_list: list) -> None:
        """
        flush data to db
        """
        self.insert_data_to_db(table_name, data_list)

    def clear(self: any) -> None:
        """
        clear db file
        :return:
        """
        try:
            if os.path.exists(PathManager.get_db_path(self.result_dir, DBNameConstant.DB_SYNC_ACL_NPU)):
                os.remove(PathManager.get_db_path(self.result_dir, DBNameConstant.DB_SYNC_ACL_NPU))
        except (OSError, SystemError) as err:
            error(os.path.basename(__file__), str(err))


class SyncAclNpuViewModel(ViewModel):
    def __init__(self: any, path: str, table_list: list) -> None:
        super().__init__(path, DBNameConstant.DB_SYNC_ACL_NPU, table_list)

    def get_torch_acl_relation_data(self):
        sql = f"select torch_op_start_time, torch_op_tid, torch_op_pid, op_name, acl_start_time, acl_end_time, " \
              f"acl_tid, acl_compile_time from {DBNameConstant.TABLE_TORCH_TO_ACL} where acl_start_time is not null " \
              f"and torch_op_start_time is not null order by acl_start_time"
        return DBManager.fetch_all_data(self.cur, sql, dto_class=TorchNpuDto)

    def get_torch_npu_relation_data(self):
        sql = f"select torch_op_start_time, torch_op_tid, torch_op_pid, stream_id, task_id, batch_id, context_id" \
              f" from {DBNameConstant.TABLE_TORCH_TO_NPU} where is_main_kernel=1"
        return DBManager.fetch_all_data(self.cur, sql, dto_class=TorchNpuDto)

