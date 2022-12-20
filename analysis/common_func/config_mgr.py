#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.

import json
import os

from common_func.common import error
from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from common_func.os_manager import check_file_readable
from common_func.msprof_exception import ProfException


class ConfigMgr:
    """
    config class
    """

    COMMON_FILE_NAME = os.path.basename(__file__)

    @staticmethod
    def get_ddr_bit_width() -> int:
        """
        get ddr bit width
        """
        platform_version = InfoConfReader().get_root_data(Constant.PLATFORM_VERSION)
        if platform_version in (Constant.CHIP_V2_1_0, Constant.CHIP_V3_1_0, Constant.CHIP_V3_2_0, Constant.CHIP_V3_3_0):
            return 256
        return 128

    @staticmethod
    def has_llc_capacity(result_dir: str) -> bool:
        """
        check whether capacity config
        """
        sample_config = ConfigMgr.read_sample_config(result_dir)
        return sample_config.get(StrConstant.LLC_PROF) == StrConstant.LLC_CAPACITY_ITEM

    @staticmethod
    def has_llc_bandwidth(result_dir: str) -> bool:
        """
        check whether bandwidth config
        """
        sample_config = ConfigMgr.read_sample_config(result_dir)
        return sample_config.get(StrConstant.LLC_PROF) == StrConstant.LLC_BAND_ITEM

    @staticmethod
    def has_llc_read_write(result_dir: str) -> bool:
        """
        check whether read or write config
        """
        sample_config = ConfigMgr.read_sample_config(result_dir)
        return sample_config.get(StrConstant.LLC_PROF) in [StrConstant.LLC_PROFILING_READ_EVENT,
                                                           StrConstant.LLC_PROFILING_WRITE_EVENT]

    @staticmethod
    def is_ai_core_sample_based(result_dir: str) -> bool:
        """
        check scene of ai core sample-based
        """
        sample_config = ConfigMgr.read_sample_config(result_dir)
        return sample_config.get(StrConstant.AICORE_PROFILING_MODE) == StrConstant.AIC_SAMPLE_BASED_MODE

    @staticmethod
    def is_ai_core_task_based(result_dir: str) -> bool:
        """
        check scene of ai core task-based
        """
        sample_config = ConfigMgr.read_sample_config(result_dir)
        return sample_config.get(StrConstant.AICORE_PROFILING_MODE) == StrConstant.AIC_TASK_BASED_MODE

    @staticmethod
    def is_aiv_sample_based(result_dir: str) -> bool:
        """
        check scene of aiv sample-based
        """
        sample_config = ConfigMgr.read_sample_config(result_dir)
        return sample_config.get(StrConstant.AIV_PROFILING_MODE) == StrConstant.AIC_SAMPLE_BASED_MODE

    @staticmethod
    def get_disk_freq(result_dir: str) -> any:
        """
        get disk profile freq
        """
        sample_config = ConfigMgr.read_sample_config(result_dir)
        return sample_config.get(StrConstant.HOST_DISK_FREQ)

    @staticmethod
    def read_sample_config(collection_path: str) -> any:
        """
        read sample config by the collection path
        :return: the sample config
        """
        sample_file = os.path.join(collection_path, Constant.SAMPLE_FILE)
        check_file_readable(sample_file)
        try:
            with open(sample_file, "r") as json_file:
                return json.load(json_file)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            message = f"Failed to load {sample_file}. {err}"
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, message) from err
        finally:
            pass
