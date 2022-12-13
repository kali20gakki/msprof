#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import argparse
import os
import sys

from common_func.common import call_sys_exit
from common_func.common import error
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from msinterface.msprof_export import ExportCommand
from msinterface.msprof_import import ImportCommand
from msinterface.msprof_monitor import monitor
from msinterface.msprof_query import QueryCommand
from msinterface.msprof_query_summary_manager import QueryDataType


class MsprofEntrance:
    """
    entrance of msprof
    """
    FILE_NAME = os.path.basename(__file__)

    @staticmethod
    def _add_collect_path_argument(parser: any) -> None:
        parser.add_argument(
            '-dir', '--collection-dir', dest='collection_path',
            default='', metavar='<dir>',
            type=str, help='<Mandatory> Specify the directory that is used for '
                           'creating data collection results.', required=True)

    @staticmethod
    def _handle_export_command(parser: any, args: any) -> None:
        if len(sys.argv) < 3:
            parser.print_help()
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        export_command = ExportCommand(sys.argv[2], args)
        export_command.process()

    @staticmethod
    def _handle_query_command(parser: any, args: any) -> None:
        _ = parser
        query_command = QueryCommand(args)
        query_command.process()

    @staticmethod
    def _handle_monitor_command(parser: any, args: any) -> None:
        _ = parser
        monitor(os.path.realpath(args.collection_path))

    @staticmethod
    def _handle_import_command(parser: any, args: any) -> None:
        _ = parser
        import_command = ImportCommand(args)
        import_command.process()

    def main(self: any) -> None:
        """
        parse argument and run command
        :return: None
        """
        parser, export_parser, import_parser, monitor_parser, query_parser = self.construct_arg_parser()

        args = parser.parse_args(sys.argv[1:])
        if len(sys.argv) < 2:
            parser.print_help()
            call_sys_exit(ProfException.PROF_INVALID_PARAM_ERROR)

        if hasattr(args, "collection_path"):
            path_len = len(os.path.realpath(args.collection_path))
            if path_len > NumberConstant.PROF_PATH_MAX_LEN:
                error(self.FILE_NAME,
                      "Please ensure the length of input dir absolute path(%s) less than %s" %
                      (path_len, NumberConstant.PROF_PATH_MAX_LEN))
                call_sys_exit(ProfException.PROF_INVALID_PARAM_ERROR)

        command_handler = {
            'export': {'parser': export_parser,
                       'handler': self._handle_export_command},
            'query': {'parser': query_parser,
                      'handler': self._handle_query_command},
            'monitor': {'parser': monitor_parser,
                        'handler': self._handle_monitor_command},
            'import': {'parser': import_parser,
                       'handler': self._handle_import_command},
        }
        handler = command_handler.get(sys.argv[1])
        try:
            handler.get('handler')(handler.get('parser'), args)
        except ProfException as err:
            if err.message:
                error(self.FILE_NAME, err)
            call_sys_exit(err.code)
        except Exception as err:
            error(self.FILE_NAME, err)
            call_sys_exit(NumberConstant.ERROR)
        call_sys_exit(ProfException.PROF_NONE_ERROR)

    def construct_arg_parser(self: any) -> tuple:
        """
        construct arg parser
        :return: tuple of parsers
        """
        parser = argparse.ArgumentParser()
        subparsers = parser.add_subparsers()
        import_parser = subparsers.add_parser(
            'import', help='Parse original profiling data by collected data.')
        export_parser = subparsers.add_parser(
            'export', help='Export profiling data by collected data.')
        query_parser = subparsers.add_parser(
            'query', help='Query specified info.')
        monitor_parser = subparsers.add_parser(
            'monitor', help="Monitor the specified directory.")
        self._query_parser(query_parser)
        self._export_parser(export_parser)
        self._monitor_parser(monitor_parser)
        self._import_parser(import_parser)
        parser_tuple = (parser, export_parser, import_parser, monitor_parser, query_parser)
        return parser_tuple

    def _query_parser(self: any, query_parser: any) -> None:
        data_type_values = list(map(int, QueryDataType))
        data_type_tips = ", ".join(map(str, data_type_values))
        self._add_collect_path_argument(query_parser)
        query_parser.add_argument(
            '--id', dest='id', default=None, metavar='<id>',
            type=int, help='<Optional> the npu device ID')
        query_parser.add_argument(
            '--data-type', dest='data_type', default=None, metavar='<data_type>',
            type=int, choices=data_type_values,
            help='<Optional> the data type to query, support {}.'.format(data_type_tips))
        query_parser.add_argument(
            '--model-id', dest='model_id', default=None, metavar='<model_id>',
            type=int, help='<Optional> the model ID')
        query_parser.add_argument(
            '--iteration-id', dest='iteration_id', default=None, metavar='<iteration_id>',
            type=int, help='<Optional> the iteration ID')

    def _add_export_argument(self: any, parser: any) -> None:
        self._add_collect_path_argument(parser)
        parser.add_argument(
            '--iteration-id', dest='iteration_id', default=None,
            metavar='<iteration_id>',
            type=int, help='<Optional> the iteration ID')
        parser.add_argument(
            '--model-id', dest='model_id', default=None,
            metavar='<model_id>',
            type=int, help='<Optional> the model ID')

    def _export_parser(self: any, export_parser: any) -> None:
        subparsers = export_parser.add_subparsers()
        summary_parser = subparsers.add_parser('summary', help='Get summary data.')
        timeline_parser = subparsers.add_parser(
            'timeline', help='Get timeline data.')
        self._add_export_argument(summary_parser)
        summary_parser.add_argument(
            '--format', dest='export_format', default='csv',
            metavar='<export_format>', choices=['csv', 'json'],
            type=str, help='<Optional> the format for export, supports csv and json.')
        self._add_export_argument(timeline_parser)

    def _import_parser(self: any, import_parser: any) -> None:
        self._add_collect_path_argument(import_parser)
        import_parser.add_argument(
            '--cluster', dest='cluster_flag',
            action='store_true', default=False,
            help='<Optional> the cluster scence flag')

    def _monitor_parser(self: any, monitor_parser: any) -> None:
        self._add_collect_path_argument(monitor_parser)
