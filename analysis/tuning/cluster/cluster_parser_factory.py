#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import os
import logging
from abc import abstractmethod


class ClusterParserFactory:
    """
    parser factory interface for cluster scene
    """
    def __init__(self: any) -> None:
        pass

    @abstractmethod
    def generate_parser(self):
        """
        generate_parse_method
        """


class ClusterCommunicationParserFactory(ClusterParserFactory):
    """
    factory which creates cluster communication parser
    provide data preparation for created parser
    """

    def __init__(self: any, params: dict) -> None:
        super().__init__()
        self.iteration_id = params.get("iteration_id")
        self.collection_path = os.path.realpath(params.get("collection_path"))

    def generate_parser(self):
        logging.info('parser_generate_success')

    def get_hccl_ops_by_iter(self):
        """
        get op events of all rank by iteration start and end time
        """
        pass


