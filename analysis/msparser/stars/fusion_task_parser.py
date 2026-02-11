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

from msparser.interface.istars_parser import IStarsParser


class FusionTaskParser(IStarsParser):
    """
      class to parse fusion task log type data
    """

    def __init__(self: any, result_dir: str, db: str, table_list: list) -> None:
        super().__init__()
        self.result_dir = result_dir
        self.db = db
        self.table_list = table_list

    def handle(self: any, _: any, data: bytes) -> None:
        """
            override, now is not support to parse fusion task log type
        """
        pass

    def preprocess_data(self: any) -> None:
        """
            Before saving to the database, subclasses can implement this method.
            Do another layer of preprocessing on the data in the buffer
            :return:result data list
        """
        pass

    def flush(self: any) -> None:
        """
            flush all buffer data to db
            :return: NA
        """
        pass