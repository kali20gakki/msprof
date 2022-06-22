#!/usr/bin/env python
# coding=utf-8
"""
function: Model of viewer for GE op execute
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from msmodel.interface.ianalysis_model import IAnalysisModel
from msmodel.interface.view_model import ViewModel


class GeOpExecuteViewModel(ViewModel, IAnalysisModel):
    """
    Model of viewer for GE op execute
    """

    def get_summary_data(self: any) -> list:
        """
        get summary data of Ge op execute
        :return:
        """
        sql = "select thread_id,op_type,event_type,start_time/{0}," \
              "(end_time-start_time)/{0} from {1}".format(NumberConstant.MILLI_SECOND,
                                                          DBNameConstant.TABLE_GE_HOST)
        if not (self.cur and self.conn):
            return []
        return DBManager.fetch_all_data(self.cur, sql)

    def get_timeline_data(self: any) -> list:
        """
        get timeline data of Ge op execute
        :return:
        """
        return self.get_summary_data()
