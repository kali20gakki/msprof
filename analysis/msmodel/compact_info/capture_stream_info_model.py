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
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel


class CaptureStreamInfoModel(ParserModel):
    """
    db capture stream info
    """

    def __init__(self: any, result_dir: str):
        super().__init__(result_dir, DBNameConstant.DB_STREAM_INFO, [DBNameConstant.TABLE_CAPTURE_STREAM_INFO])

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_CAPTURE_STREAM_INFO) -> None:
        """
        insert data into database
        """
        if not self.table_list:
            return
        self.insert_data_to_db(table_name, data_list)
