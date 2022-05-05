#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to parser
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""
from abc import abstractmethod

from model.interface.base_model import BaseModel


class ParserModel(BaseModel):
    """
    class used to calculate
    """

    @abstractmethod
    def flush(self: any, data_list: list) -> None:
        """
        base method to insert data into database
        """

    def init(self: any) -> bool:
        """
        create db and tables
        """
        if not super().init():
            return False
        self.create_table()
        return True
