#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class NanoPmuDto(metaclass=InstanceCheckMeta):
    """
    Dto for nano pmu data
    """
    block_dim = None
    pmu0 = None
    pmu1 = None
    pmu2 = None
    pmu3 = None
    pmu4 = None
    pmu5 = None
    pmu6 = None
    pmu7 = None
    pmu8 = None
    pmu9 = None
    stream_id = None
    task_id = None
    total_cycle = None

    @property
    def pmu_list(self: any) -> any:
        return [self._pmu0, self._pmu1, self._pmu2, self._pmu3,
                self._pmu4, self._pmu5, self._pmu6, self._pmu7,
                self._pmu8, self._pmu9]
