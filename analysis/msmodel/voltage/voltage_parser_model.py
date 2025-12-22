#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

import logging

from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel


class VoltageParserModel(ParserModel):
    """
    base voltage parser model
    """

    def flush(self: any, data_list: list) -> None:
        """
        insert data into database
        """
        if self.table_list:
            self.insert_data_to_db(self.table_list[0], data_list)


class AicVoltageParserModel(VoltageParserModel):
    """
    db operator for aic voltage parser
    """

    def __init__(self: any, result_dir: str) -> None:
        super(AicVoltageParserModel, self).__init__(result_dir, DBNameConstant.DB_VOLTAGE,
                                                    [DBNameConstant.TABLE_AIC_VOLTAGE])


class BusVoltageParserModel(VoltageParserModel):
    """
    db operator for bus voltage parser
    """

    def __init__(self: any, result_dir: str) -> None:
        super(BusVoltageParserModel, self).__init__(result_dir, DBNameConstant.DB_VOLTAGE,
                                                    [DBNameConstant.TABLE_BUS_VOLTAGE])

