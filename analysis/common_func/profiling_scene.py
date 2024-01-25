#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2023. All rights reserved.

import logging

from common_func.constant import Constant
from common_func.singleton import singleton
from common_func.utils import Utils


@singleton
class ProfilingScene:
    """
    Differentiate the scene for the current profiling data
    """

    def __init__(self: any) -> None:
        self.project_path = None
        self._scene = None
        self._all_export = True
        self._step_export = False

    def set_all_export(self: any, value: bool) -> None:
        self._all_export = value

    def set_value(self: any, **kwargs) -> None:
        self._all_export = kwargs.get("all_export", self._all_export)
        self._step_export = kwargs.get("step_export", self._step_export)

    def init(self: any, project_path: str) -> None:
        """
        init the class profiling scene and load the data from the current data
        :param project_path: project path
        :return: NA
        """
        self.project_path = project_path
        self._scene = None

    def get_scene(self: any) -> any:
        """
        get scene of the current data
        :return: scene
        """
        if self._scene is None:
            self._scene = Utils.get_scene(self.project_path)
            logging.info("Current scene of data is based on %s", self._scene)
        return self._scene

    def is_all_export(self: any) -> bool:
        """
        check whether all export
        :return: True or False
        """
        return self._all_export

    def is_step_export(self: any) -> bool:
        """
        check whether step export
        :return: True or False
        """
        # 按step导数据必须要支持全导
        return self._all_export and self._step_export

    def is_operator(self: any) -> bool:
        """
        check whether operator
        :return: True or False
        """
        return self.get_scene() == Constant.SINGLE_OP

    def is_mix_operator_and_graph(self: any) -> bool:
        """
        check whether operator
        :return: True or False
        """
        return self.get_scene() == Constant.MIX_OP_AND_GRAPH