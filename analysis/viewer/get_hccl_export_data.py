#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
import json
from collections import OrderedDict

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from common_func.utils import Utils
from msmodel.hccl.hccl_model import HcclViewModel
from profiling_bean.db_dto.hccl_dto import HcclDto


class HCCLExport:
    """
    hccl export
    """
    HCCL_SORTED_OFFSET = 70000

    def __init__(self: any, param: dict) -> None:
        self.project_path = param.get(StrConstant.PARAM_RESULT_DIR)
        self.result = []
        self.err_message = {}
        self.iter_range = param.get(StrConstant.PARAM_ITER_ID)
        self.pid_value = InfoConfReader().get_json_pid_data()

    def get_hccl_timeline_data(self: any) -> str:
        """
        get data for hccl timeline
        """
        with HcclViewModel(self.project_path, DBNameConstant.DB_HCCL,
                           [DBNameConstant.TABLE_HCCL_ALL_REDUCE]) as hccl_model:
            if not hccl_model.check_table():
                return json.dumps({
                    'status': NumberConstant.ERROR,
                    "info": "get hccl data failed, may be the hccl file not completed or hccl parser failed."
                            " please check data file."
                })

            hccl_data = hccl_model.get_all_data(DBNameConstant.TABLE_HCCL_ALL_REDUCE, dto_class=HcclDto)
            if not hccl_data:
                return json.dumps({
                    'status': NumberConstant.WARN,
                    "info": f"get hccl data failed, "
                            f"may be lack of hccl files containing iteration {self.iter_range.iteration_id}."
                })
        self._format_hccl_data(hccl_data)
        return json.dumps(self.result)

    def _get_meta_data(self: any, hccl_data: list) -> None:
        self.result = TraceViewManager.metadata_event(
            [["process_name", self.pid_value, InfoConfReader().get_json_tid_data(), "HCCL"]])
        tid_list_set = set(map(lambda x: x.plane_id, hccl_data))
        tid_list = sorted(tid_list_set)
        self.result.extend(TraceViewManager.metadata_event(
            Utils.generator_to_list(
                ["thread_name", self.pid_value, tid_value, "Plane {}".format(tid_value)]
                for tid_value in tid_list
            )))
        self.result.extend(TraceViewManager.metadata_event(Utils.generator_to_list(
            ["thread_sort_index", self.pid_value, tid_value, tid_value + self.HCCL_SORTED_OFFSET]
            for tid_value in tid_list
        )))

        # In order to show Communication OP on the top of HCCL process.
        self.result.extend(TraceViewManager.metadata_event(
            [["thread_name", self.pid_value, self.HCCL_SORTED_OFFSET, "Communication Kernel"]]))
        self.result.extend(TraceViewManager.metadata_event(
            [["thread_sort_index", self.pid_value, self.HCCL_SORTED_OFFSET, 0]]))

    def _format_hccl_data(self: any, hccl_data: list) -> None:
        self._get_meta_data(hccl_data)
        _hccl_format_data = self._format_hccl_communication_data(hccl_data)
        _hccl_format_op_data = self._format_hccl_op_data()
        self.result.extend(TraceViewManager.time_graph_trace(
            TraceViewHeaderConstant.GRPC_TIME_GRAPH_HEAD, _hccl_format_data + _hccl_format_op_data))

    def _format_hccl_op_data(self):
        with HcclViewModel(self.project_path, DBNameConstant.DB_HCCL,
                           DBNameConstant.TABLE_HCCL_ALL_REDUCE) as hccl_model:
            hccl_op_data = hccl_model.get_hccl_op_data()
            _hccl_format_op_data = [
                [
                    hccl_op.op_name, self.pid_value, self.HCCL_SORTED_OFFSET,
                    hccl_op.timestamp / NumberConstant.NS_TO_US, hccl_op.duration / NumberConstant.NS_TO_US
                ]
                for hccl_op in hccl_op_data
            ]
        return _hccl_format_op_data

    def _format_hccl_communication_data(self, hccl_data):
        _hccl_format_data = [0] * len(hccl_data)
        for index, _hccl_data in enumerate(hccl_data):
            hccl_args = OrderedDict(_hccl_data.args)
            _hccl_data_pice = [
                _hccl_data.hccl_name, self.pid_value, _hccl_data.plane_id,
                _hccl_data.timestamp / NumberConstant.NS_TO_US, _hccl_data.duration / NumberConstant.NS_TO_US, hccl_args
            ]
            _hccl_format_data[index] = _hccl_data_pice
        return _hccl_format_data
