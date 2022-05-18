"""
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

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
        self._mix_scene = None

    def init(self: any, project_path: str) -> None:
        """
        init the class profiling scene and load the data from the current data
        :param project_path: project path
        :return: NA
        """
        self.project_path = project_path
        self._scene = None
        self._mix_scene = None

    def get_scene(self: any) -> any:
        """
        get scene of the current data
        :return: scene
        """
        if self._scene is None:
            self._scene = Utils.get_scene(self.project_path)
            logging.info("Current scene of data is based on %s", self._scene)
        return self._scene

    def get_mix_scene(self: any) -> any:
        """
        get scene of the current data
        :return: scene
        """
        if self._mix_scene is None:
            self._mix_scene = Utils.is_single_op_graph_mix(self.project_path)
        return self._mix_scene

    def is_step_trace(self: any) -> bool:
        """
        check whether step trace
        :return: True or False
        """
        return self.get_scene() == Constant.STEP_INFO

    def is_training_trace(self: any) -> bool:
        """
        check whether training trace
        :return: True or False
        """
        return self.get_scene() == Constant.TRAIN

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
        return self.get_mix_scene()
