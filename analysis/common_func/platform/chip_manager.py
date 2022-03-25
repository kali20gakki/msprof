# coding=utf-8
"""
Function:
DataCheckManager class, This class mainly involves chip info.
Copyright Information:
Huawei Technologies Co., Ltd. All Rights Reserved © 2021
"""
import os
import sys

from common_func.common import error
from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from common_func.singleton import singleton
from profiling_bean.prof_enum.chip_model import ChipModel


@singleton
class ChipManager:
    """
    class used to get chip info.
    """
    CHIP_RELATION_MAP = {
        Constant.CHIP_V1_1_0: ChipModel.CHIP_V1_1_0,
        Constant.CHIP_V2_1_0: ChipModel.CHIP_V2_1_0,
        Constant.CHIP_V3_1_0: ChipModel.CHIP_V3_1_0,
        Constant.CHIP_V3_2_0: ChipModel.CHIP_V3_2_0,
        Constant.CHIP_V3_3_0: ChipModel.CHIP_V3_3_0,
        Constant.CHIP_V4_1_0: ChipModel.CHIP_V4_1_0
    }

    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any) -> None:
        self.chip_id = ChipModel.CHIP_V1_1_0

    @staticmethod
    def is_ffts_type() -> bool:
        """
        check if ffts type
        :return: True or False
        """
        return True

    @staticmethod
    def is_ffts_plus_type() -> bool:
        """
        check if ffts plus type
        :return:
        """
        return True

    @classmethod
    def get_chip_id(cls: any) -> any:
        """
        get chip id
        :return: chip id
        """
        return cls.CHIP_RELATION_MAP.get(InfoConfReader().get_root_data(Constant.PLATFORM_VERSION))

    def load_chip_info(self: any) -> None:
        """
        load chip info
        :return:
        """
        if InfoConfReader().get_root_data(Constant.PLATFORM_VERSION) not in self.CHIP_RELATION_MAP.keys():
            error(self.FILE_NAME, "Can't get platform version or platform version isn't identified from info.json, "
                                  "please check the file.")
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        self.chip_id = self.CHIP_RELATION_MAP.get(InfoConfReader().get_root_data(Constant.PLATFORM_VERSION))

    def is_chip_v1(self: any) -> bool:
        """
        check the scene of mini
        :return: True or False
        """
        return self.chip_id == ChipModel.CHIP_V1_1_0

    def is_chip_v2(self: any) -> bool:
        """
        check the scene of cloud
        :return: True or False
        """
        return self.chip_id == ChipModel.CHIP_V2_1_0

    def is_chip_v3(self: any) -> bool:
        """
        check the scene of dc or mdc or lhisi
        :return: True or False
        """
        return self.chip_id in (ChipModel.CHIP_V3_1_0, ChipModel.CHIP_V3_2_0, ChipModel.CHIP_V3_3_0)

    def is_chip_v3_1(self: any) -> bool:
        """
        check the scene of mdc
        :return: True or False
        """
        return self.chip_id == ChipModel.CHIP_V3_1_0

    def is_chip_v3_2(self: any) -> bool:
        """
        check the scene of lhisi
        :return: True or False
        """
        return self.chip_id == ChipModel.CHIP_V3_2_0

    def is_chip_v3_3(self: any) -> bool:
        """
        check the scene of dc
        :return: True or False
        """
        return self.chip_id == ChipModel.CHIP_V3_3_0

    def is_chip_v4(self: any) -> bool:
        """
        check the scene of chip.v4.1.0
        :return: True or False
        """
        return self.chip_id == ChipModel.CHIP_V4_1_0
