#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.msvp_common import path_check
from common_func.path_manager import PathManager
from msmodel.ge.ge_info_calculate_model import GeInfoModel
from msmodel.step_trace.ts_track_model import TsTrackModel
from msparser.iter_rec.iter_info_updater.iter_info import IterInfo


class IterInfoManager:
    def __init__(self: any, project_path: str) -> None:
        self.project_path = project_path
        self.iter_to_iter_info = {}
        self.is_parallel_scene = False
        self.initial_iter_to_info()

    def initial_iter_to_info(self: any) -> None:
        if not path_check(PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_INFO)):
            return
        with GeInfoModel(self.project_path) as ge_info_model:
            step_trace_data = ge_info_model.get_step_trace_data()
            static_task_dict = ge_info_model.get_ge_task_data(Constant.TASK_TYPE_AI_CORE, Constant.GE_STATIC_SHAPE)
            dynamic_task_dict = ge_info_model.get_ge_task_data(Constant.TASK_TYPE_AI_CORE, Constant.GE_NON_STATIC_SHAPE)

        self.regist_paralle_set(step_trace_data)
        self.regist_aicore_set(static_task_dict, dynamic_task_dict)

    def regist_paralle_set(self: any, step_trace_data: list) -> None:
        for index, step_trace_datum in enumerate(step_trace_data):
            iter_info = self.iter_to_iter_info.setdefault(step_trace_datum.iter_id,
                                                          IterInfo(step_trace_datum.model_id,
                                                                   step_trace_datum.index_id,
                                                                   step_trace_datum.iter_id,
                                                                   step_trace_datum.step_start,
                                                                   step_trace_datum.step_end))
            for behind_datum in step_trace_data[index:]:
                behind_iter_info = self.iter_to_iter_info.setdefault(behind_datum.iter_id,
                                                                     IterInfo(behind_datum.model_id,
                                                                              behind_datum.index_id,
                                                                              behind_datum.iter_id,
                                                                              behind_datum.step_start,
                                                                              behind_datum.step_end))
                if behind_iter_info.start_time < iter_info.end_time <= behind_iter_info.end_time:
                    iter_info.behind_parallel_iter.add(behind_datum.iter_id)

    def regist_aicore_set(self: any, static_task_dict: dict, dynamic_task_dict: dict) -> None:
        for iter_info_bean in self.iter_to_iter_info.values():
            iter_info_bean.static_aic_task_set = static_task_dict.get(iter_info_bean.model_id, set([]))
            iter_info_bean.dynamic_aic_task_set = dynamic_task_dict.get(
                GeInfoModel.MODEL_INDEX_KEY_FMT.format(
                    iter_info_bean.model_id,
                    iter_info_bean.index_id),
                set([]))

    @classmethod
    def check_parallel(cls: any) -> bool:
        if not DBManager.check_tables_in_db(PathManager.get_db_path(self.result_dir, DBNameConstant.DB_STEP_TRACE),
                                            DBNameConstant.TABLE_STEP_TRACE_DATA):
            return False
        ts_track_model = TsTrackModel(
            project_path, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE_DATA])

        with ts_track_model:
            check_res = ts_track_model.check_parallel()

        if check_res:
            return True
        return False