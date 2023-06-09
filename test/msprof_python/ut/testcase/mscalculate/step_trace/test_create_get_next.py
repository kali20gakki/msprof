#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import unittest
from unittest import mock
from mscalculate.step_trace.create_step_table import GetNextCreator
from profiling_bean.db_dto.step_trace_dto import StepTraceOriginDto

NAMESPACE = 'mscalculate.step_trace.create_step_table'


def get_data():
    data = [
        StepTraceOriginDto(),
        StepTraceOriginDto(),
        StepTraceOriginDto(),
        StepTraceOriginDto(),
        StepTraceOriginDto(),
        StepTraceOriginDto(),
    ]
    timestamp = 189746300646091
    model_ids = [1, 1, 2, 2, 2, 2]
    tag_ids = [20000, 20001, 20000, 20001, 2, 20000]
    for step_trace, model_id, tag_id in zip(data, model_ids, tag_ids):
        step_trace.index_id = 1
        step_trace.model_id = model_id
        step_trace.timestamp = timestamp
        step_trace.stream_id = 20
        step_trace.task_id = 5
        step_trace.tag_id = tag_id
        timestamp += 10
    return data


class TestCreateGetNext(unittest.TestCase):
    def test_run(self):
        data = get_data()
        sample_config = {"result_dir": "./", "devices": 0}
        with mock.patch(NAMESPACE + '.DBManager.execute_sql'), \
                mock.patch(NAMESPACE + '.DBManager.executemany_sql'):
            GetNextCreator.run(sample_config, data)
            self.assertEqual(GetNextCreator.data, [[1, 20, 1, 189746300646091, 189746300646101],
                                                   [2, 20, 1, 189746300646111, 189746300646121],
                                                   [2, 20, 2, 189746300646141, 0],
                                                   ])


if __name__ == '__main__':
    unittest.main()
