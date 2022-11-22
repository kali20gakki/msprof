#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


import logging
from abc import abstractmethod


class ClusterCalculatorFactory:
    """
    calculator factory interface for cluster scene
    """
    def __init__(self: any) -> None:
        pass

    @abstractmethod
    def generate_calculator(self):
        """
        generate calculator method
        """


class SlowRankCalculatorFactory(ClusterCalculatorFactory):
    """
    calculator factory for looking for slow rank
    """

    def __init__(self: any, data: dict) -> None:
        super().__init__()
        self.data = data

    def data_dispatch(self: any) -> None:
        """
        input data contains iteration and different ops data
        we need dispatch first then process
        """
        pass

    def generate_calculator(self):
        logging.info('calculator1_generate_success')


class SlowLinkCalculatorFactory(ClusterCalculatorFactory):
    """
    calculator factory for looking for slow link
    """

    def __init__(self: any, data: dict) -> None:
        super().__init__()
        self.data = data

    def data_dispatch(self: any) -> None:
        """
        input data contains iteration and different ops data
        we need dispatch first then process
        """
        pass

    def generate_calculator(self):
        logging.info('calculator2_generate_success')

