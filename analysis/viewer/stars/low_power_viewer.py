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

import itertools
import logging
from abc import ABC
from typing import List, Dict

from common_func.info_conf_reader import InfoConfReader
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.stars.low_power_model import LowPowerViewModel
from viewer.interface.base_viewer import BaseViewer


class LowPowerViewer(BaseViewer, ABC):
    """
    class for get lowpower sample data
    （1）Ddie有10个硬件数据：0-I2C_IMON回读；1-AVSBUS_BUS回读；2-AVSBUS_AIC回读；3-AIC结温；4-非AIC结温；5-MEM_DRAM_DIE结温；
        6-软件DVFS下发的AIC频率（不使用，通过软件ch10上报）；7-PPU上报的AIC平均频率；8-软件DVFS下发的BUS频率（不使用，
        通过软件ch11上报）；9-PPU上报的BUS平均频率；
        6个软件数据：10-软件DVFS下发的AIC频率；11-软件DVFS下发的BUS频率；12-EDP降频POWERBRAKE计数；13-EDP降频IWARNING2计数;
        14-EDP降频IWARNING1计数;15-EDP降频IWARNING0计数;
    （2）Udie有9个硬件数据：0-Udie结温；1-PMBUS0的回读数据0；2-PMBUS0的回读数据1；3-PMBUS1的回读数据0；4-PMBUS1的回读数据1；
        5-PMBUS2的回读数据0；6-PMBUS2的回读数据1；7-PMBUS3的回读数据0；8-PMBUS3的回读数据1；
    """
    LOW_POWER_D_DIE_HARD = ['I2C_IMON', 'AVSBUS_BUS', 'AVSBUS_AIC', 'AIC_TEMP', 'Non-AIC_TEMP', 'MEM_DRAM_DIE_TEMP',
                            'USELESS_DVFS_AIC_Freq', 'PPU_AIC_AVG_Freq', 'USELESS_DVFS_BUS_Freq', 'PPU_BUS_AVG_Freq']
    LOW_POWER_D_DIE_SOFT = ['DVFS_AIC_Freq', 'DVFS_BUS_Freq', 'EDP_POWERBRAKE', 'EDP_IWARNING2', 'EDP_IWARNING1',
                            'EDP_IWARNING0']
    LOW_POWER_U_DIE_HARD = ['Udie_TEMP', 'PMBUS0_Data0', 'PMBUS0_Data1', 'PMBUS1_Data0', 'PMBUS1_Data1',
                            'PMBUS2_Data0', 'PMBUS2_Data1', 'PMBUS3_Data0', 'PMBUS3_Data1']
    LOW_POWER_U_DIE_SOFT = []
    D_DIE_ID_LIST = [0, 1]
    U_DIE_ID_LIST = [2, 3]
    DIE_ID_KEY = {
        0: "die 0",
        1: "die 1",
        2: "die 2",
        3: "die 3",
    }

    def __init__(self: any, configs: dict, params: dict) -> None:
        super().__init__(configs, params)
        self.model_list = {
            'low_power': LowPowerViewModel
        }

    def get_trace_timeline(self: any, datas: list) -> list:
        """
        format data to standard timeline format
        :return: list
        """
        timestamp_index = 0
        die_id_index = 1
        low_power_hard_index = 2
        low_power_soft_index = 12
        args_index = 4
        pid = InfoConfReader().get_json_pid_data()
        tid = InfoConfReader().get_json_tid_data()
        meta_data = [['process_name', pid, tid, 'Low Power']]
        result = TraceViewManager.metadata_event(meta_data)
        event_dict = {}
        for data in datas:
            local_time = InfoConfReader().trans_into_local_time(raw_timestamp=data[timestamp_index])
            die_id = data[die_id_index]
            if die_id in self.D_DIE_ID_LIST:
                lp_type_hard = self.LOW_POWER_D_DIE_HARD
                lp_type_soft = self.LOW_POWER_D_DIE_SOFT
            elif die_id in self.U_DIE_ID_LIST:
                lp_type_hard = self.LOW_POWER_U_DIE_HARD
                lp_type_soft = self.LOW_POWER_U_DIE_SOFT
            else:
                logging.error(f"Unexpected die id: {die_id}.")
                continue
            for key, value in itertools.chain(zip(lp_type_hard, data[low_power_hard_index:low_power_soft_index]),
                                              zip(lp_type_soft, data[low_power_soft_index:])):
                if key.startswith('USELESS'):
                    continue
                tmp_key = "{}_{}".format(key, local_time)
                if tmp_key not in event_dict:
                    event_dict[tmp_key] = [key, local_time, pid, tid, {self.DIE_ID_KEY.get(die_id): value}]
                else:
                    event_dict.get(tmp_key)[args_index][self.DIE_ID_KEY.get(die_id)] = value
        column_trace_data = list(event_dict.values())
        result.extend(TraceViewManager.column_graph_trace(TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST,
                                                          column_trace_data))
        return result
