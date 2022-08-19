# coding=utf-8
"""
function: this script used to operate AI_CPU
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""


class RuntimeModelInfo:
    db_class = RuntimeModel
    table_name = TaskType
    start_tag = 3
    end_tag = 4


class TsTrackModelInfo:
    db_class = TsTrackModel
    table_name = TaskType
    start_tag = 1
    end_tag = 2

