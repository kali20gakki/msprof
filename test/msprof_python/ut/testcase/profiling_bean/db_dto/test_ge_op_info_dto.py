#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.

import unittest

from profiling_bean.db_dto.ge_op_info_dto import GeOpInfoDto


class TestGeOpInfoDto(unittest.TestCase):

    def test_ge_op_info_dto(self):
        ge_op_info_dto = GeOpInfoDto()
        ge_op_info_dto.model_id = 1
        ge_op_info_dto.task_id = 2
        ge_op_info_dto.stream_id = 3
        ge_op_info_dto.op_name = 4
        ge_op_info_dto.op_type = 5
        ge_op_info_dto.task_type = 6
        ge_op_info_dto.start_time = 7
        ge_op_info_dto.duration_time = 8

        self.assertEqual(
            (ge_op_info_dto.model_id,
             ge_op_info_dto.task_id,
             ge_op_info_dto.stream_id,
             ge_op_info_dto.op_name,
             ge_op_info_dto.op_type,
             ge_op_info_dto.task_type,
             ge_op_info_dto.start_time,
             ge_op_info_dto.duration_time),
            (1, 2, 3, 4, 5, 6, 7, 8)
        )

