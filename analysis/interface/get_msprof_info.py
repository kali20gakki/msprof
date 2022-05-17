# coding=utf-8
"""
Function: The entry for constructing the basic info.
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import argparse
import importlib
import os
import sys


class MsprofInfoConstruct:
    """
    get basic info
    get basic info
    """
    BASIC_MODEL_PATH = "profiling_bean.basic_info.msprof_basic_info"
    BASIC_INFO_CLASS_NAME = "MsProfBasicInfo"

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

    def load_basic_info_model(self: any, args: any) -> None:
        """
        load model of basic info class
        :param args: collection path
        :return: None
        """
        if not hasattr(args, "collection_path"):
            return

        model_obj = importlib.import_module(self.BASIC_MODEL_PATH)
        if hasattr(model_obj, self.BASIC_INFO_CLASS_NAME):
            msprof_basic_info = getattr(model_obj, self.BASIC_INFO_CLASS_NAME)(args.collection_path)
            msprof_basic_info.init()
            print(msprof_basic_info.run())

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
    sys.path.insert(0, os.path.realpath(os.path.join(os.path.dirname(__file__), '..')))
    MsprofInfoConstruct().main()
