#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
from dataclasses import dataclass


@dataclass
class NanoPmuDto:
    """
    Dto for nano pmu data
    """

    stream_id: int = None
    task_id: int = None
    total_cycle: int = None
    block_dim: int = None
    pmu0: float = None
    pmu1: float = None
    pmu2: float = None
    pmu3: float = None
    pmu4: float = None
    pmu5: float = None
    pmu6: float = None
    pmu7: float = None
    pmu8: float = None
    pmu9: float = None

    @property
    def pmu_list(self: any) -> any:
        return [self._pmu0, self._pmu1, self._pmu2, self._pmu3,
                self._pmu4, self._pmu5, self._pmu6, self._pmu7,
                self._pmu8, self._pmu9]
