# coding=utf-8
"""
function: script used to defining constants of stars
Copyright Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
"""


class StarsConstant:
    """
    class used to record constant about stars
    """
    ACSQ_START_FUNCTYPE = '000000'
    ACSQ_END_FUNCTYPE = '000001'
    FFTS_LOG_START_TAG = '100010'
    FFTS_LOG_END_TAG = '100011'

    # chip trans type
    TYPE_STARS_PA = "100010"
    TYPE_STARS_PCIE = "111111"

    FFTS_TYPE = {
        0: "AIC only",
        1: "AIV only",
        2: "automatic threading mode",
        3: "manual threading mode",
        4: "ffts+"
    }
    SUBTASK_TYPE = {
        0: "AIC",
        1: "AIV",
        3: "notify_wait",
        4: "notify_record",
        5: "write_value",
        6: "MIX_AIC",
        7: "MIX_AIV",
        8: "SDMA",
        9: "data context",
        # Schedule the DMU descriptor to SDMA, and SDMA informs the L2 cache to
        #   invalidate the corresponding cache line directly
        10: "invalidate data context",
        # FFTS schedules the DMU descriptor to SDMA,
        # and SDMA informs L2 cache to write back the corresponding cache line
        11: "writeback data context",
        12: "AICPU",
        13: "Load context"
    }

    def get_subtask_type(self: any) -> dict:
        """
        get subtask type
        :return: dict
        """
        return self.SUBTASK_TYPE

    def get_ffts_type(self: any) -> dict:
        """
        get ffts type
        :return: dict
        """
        return self.FFTS_TYPE
