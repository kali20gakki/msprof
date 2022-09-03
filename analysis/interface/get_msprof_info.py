#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import argparse
import importlib
import os
import sys


class MsprofInfoConstruct:
    """
    get basic info
    """
    BASIC_MODEL_PATH = "profiling_bean.basic_info.msprof_basic_info"
    BASIC_INFO_CLASS_NAME = "MsProfBasicInfo"
    CLUSTER_INFO_MODEL_PATH = "profiling_bean.basic_info.msprof_cluster_info"
    CLUSTER_INFO_CLASS_NAME = "MsProfClusterInfo"

    @staticmethod
    def construct_argument_parser() -> argparse.ArgumentParser:
        """
        construct argument parser for basic info
        :return: arg parser
        """
        parser = argparse.ArgumentParser()
        parser.add_argument(
            '-dir', '--collection-dir', dest='collection_path',
            default='', metavar='<dir>',
            type=str, help='<Mandatory> Specify the directory that is used for '
                           'creating data collection results.', required=True)
        return parser

    @staticmethod
    def _check_cluster_sqlite(path: str) -> bool:
        from common_func.db_name_constant import DBNameConstant
        from common_func.path_manager import PathManager
        path = os.path.realpath(path)
        return os.path.exists(PathManager.get_db_path(path, DBNameConstant.DB_CLUSTER_RANK)) \
               and os.path.exists(PathManager.get_db_path(path, DBNameConstant.DB_CLUSTER_STEP_TRACE))

    def load_basic_info_model(self: any, args: any) -> None:
        """
        load model of basic info class
        :param args: collection path
        :return: None
        """
        from common_func.common import print_msg
        if not hasattr(args, "collection_path"):
            return

        if MsprofInfoConstruct._check_cluster_sqlite(args.collection_path):
            model_obj = importlib.import_module(self.CLUSTER_INFO_MODEL_PATH)
            if hasattr(model_obj, self.CLUSTER_INFO_CLASS_NAME):
                msprof_cluster_info = getattr(model_obj, self.CLUSTER_INFO_CLASS_NAME)(args.collection_path)
                msprof_cluster_info.run()
        else:
            model_obj = importlib.import_module(self.BASIC_MODEL_PATH)
            if hasattr(model_obj, self.BASIC_INFO_CLASS_NAME):
                msprof_basic_info = getattr(model_obj, self.BASIC_INFO_CLASS_NAME)(args.collection_path)
                msprof_basic_info.init()
                print_msg(msprof_basic_info.run())

    def main(self: any) -> None:
        """
        interface entry for basic info
        :return:None
        """
        parser = self.construct_argument_parser()
        if len(sys.argv) < 2:
            parser.print_help()
            return

        args = parser.parse_args(sys.argv[1:])
        self.load_basic_info_model(args)


if __name__ == '__main__':
    sys.path.append(os.path.realpath(os.path.join(os.path.dirname(__file__), '..')))
    MsprofInfoConstruct().main()
