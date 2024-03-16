#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024. All rights reserved.

from collections import OrderedDict
import logging
from common_func.ms_constant.number_constant import NumberConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.platform.chip_manager import ChipManager
from common_func.profiling_scene import ProfilingScene
from msinterface.msprof_timeline import MsprofTimeline
from msmodel.freq.freq_data_viewer_model import FreqDataViewModel
from msmodel.api.api_data_viewer_model import ApiDataViewModel


class AiCoreFreqViewer:
    '''
    Read aicore freq and generate aicore freq view
    '''
    def __init__(self, params: dict):
        self._params = params
        self._project_path = params.get(StrConstant.PARAM_RESULT_DIR)
        self._pid = InfoConfReader().get_json_pid_data()
        self.freq_model = FreqDataViewModel(params)
        self.apidata_model = ApiDataViewModel(params)

    def get_start_time(self) -> str:
        if not ProfilingScene().is_all_export():
            start_time, _ = MsprofTimeline().get_start_end_time()
            return str(start_time)
        with self.apidata_model as _model:
            apidata = _model.get_earliests_api()
            if not apidata:
                logging.warning("Fetching apidata starttime result is none! \
                    aicore freq start time will been set device start.info begin time")
                start_ts, _ = InfoConfReader().get_collect_time()
                return start_ts
            return InfoConfReader().trans_into_local_time(
                InfoConfReader().time_from_host_syscnt(apidata[0].start, NumberConstant.MICRO_SECOND), use_us=True)

    def get_all_data(self):
        '''
        1、read the fixed aicore freq from info.json
        2、read the floating aicore freq from freq.db
        3、concat 1/2 freq
        '''
        result = []
        if not ChipManager().is_chip_v4() or InfoConfReader().is_host_profiling():
            return result

        # add header for freq view
        result.extend(TraceViewManager.metadata_event([["process_name", self._pid, \
            InfoConfReader().get_json_tid_data(), \
            TraceViewHeaderConstant.PROCESS_AI_CORE_FREQ]]))
        # freq unit is MHZ, read from info.json
        freq_lists = []       
        data_list = [
            TraceViewHeaderConstant.PROCESS_AI_CORE_FREQ, self.get_start_time(), self._pid, 0,
            OrderedDict({"MHz": InfoConfReader().get_freq("aic") / NumberConstant.FREQ_TO_MHz})
        ]
        freq_lists.append(data_list)

        # get freq from freq.db
        with self.freq_model as _model:
            freq_rows = _model.get_data()
            for row in freq_rows:
                # row index 0 is syscnt, and row index 1 is freqency
                to_local_ts = InfoConfReader().trans_syscnt_into_local_time(row[0])
                data_list = [
                    TraceViewHeaderConstant.PROCESS_AI_CORE_FREQ,
                    to_local_ts, self._pid, 0,
                    OrderedDict({"MHz": row[1]})
                ]
                freq_lists.append(data_list)
        _, end_ts = InfoConfReader().get_collect_time()
        data_last = freq_lists[-1]
        data_list = [
            data_last[0], end_ts, self._pid, 0,
            data_last[4]
        ]
        freq_lists.append(data_list)
        changed_frequency = TraceViewManager.column_graph_trace(
            TraceViewHeaderConstant.COLUMN_GRAPH_HEAD_LEAST, freq_lists)
        result.extend(changed_frequency)

        return result
