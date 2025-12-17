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
from common_func.ms_constant.stars_constant import StarsConstant
from msmodel.interface.parser_model import ParserModel


class StarsChipTransModel(ParserModel):
    """
    stars model class
    """

    TYPE_TABLE_MAP = {
        StarsConstant.TYPE_STARS_PA: DBNameConstant.TABLE_STARS_PA_LINK,
        StarsConstant.TYPE_STARS_PCIE: DBNameConstant.TABLE_STARS_PCIE
    }

    def __init__(self: any, result_dir: str, db: str, table_list: list) -> None:
        super().__init__(result_dir, db, table_list)

    def flush(self: any, data_dict: dict) -> None:
        """
        insert stars data into database
        """
        for _type in data_dict.keys():
            if not self.TYPE_TABLE_MAP.get(_type):
                continue
            self.insert_data_to_db(self.TYPE_TABLE_MAP.get(_type), data_dict.get(_type))
