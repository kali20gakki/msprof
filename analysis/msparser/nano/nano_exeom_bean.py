#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
from msparser.add_info.add_info_bean import AddInfoBean


class NanoExeomBean(AddInfoBean):
    """
    Nano Exeom bean data for the data parsing.
    """

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        filed = args[0]
        self._model_id = filed[6]
        self._model_name = filed[7]

    @property
    def graph_id(self: any) -> int:
        """
        nano host data model_id
        :return: model_id
        """
        return self._model_id

    @property
    def model_name(self: any) -> str:
        """
        nano host data model_name
        :return: model_name
        """
        return str(self._model_name)
