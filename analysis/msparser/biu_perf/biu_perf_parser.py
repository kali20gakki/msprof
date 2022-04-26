# coding=utf-8
"""
function: script used to parse biu perf data and save it to db
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import logging

from common_func.ms_multi_process import MsMultiProcess
from common_func.ms_constant.str_constant import StrConstant
from common_func.db_name_constant import DBNameConstant
from msparser.interface.iparser import IParser
from model.biu_perf.biu_perf_model import BiuPerfModel
from msparser.biu_perf.biu_core_parser import BiuCubeParser
from msparser.biu_perf.biu_core_parser import BiuVectorParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.biu_perf.core_info_bean import CoreInfo


class BiuPerfParser(IParser, MsMultiProcess):

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = self._sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._model = BiuPerfModel(self._project_path,
                                   [DBNameConstant.TABLE_MONITOR0,
                                    DBNameConstant.TABLE_MONITOR1])
        self._monitor0_data_list = []
        self._monitor1_data_list = []

    def ms_run(self: any) -> None:
        """
        parse biu perf data and save it to db.
        :return:None
        """
        if self._file_list.get(DataTag.BIU_PERF, []):
            self.parse()
            self.save()

    def parse(self: any) -> None:
        """
        to read biu perf data
        :return: None
        """
        biu_perf_file_dict = self._file_group_by_core()
        for core_info in biu_perf_file_dict.values():
            if core_info.core_type == CoreInfo.AI_CUBE:
                monitor1_data, monitor0_data = \
                    BiuCubeParser(self._sample_config, core_info).get_monitor_data()
                self._monitor1_data_list.extend(monitor1_data)
                self._monitor0_data_list.extend(monitor0_data)
            elif core_info.core_type in [CoreInfo.AI_VECTOR0, CoreInfo.AI_VECTOR1]:
                monitor1_data = BiuVectorParser(self._sample_config, core_info).get_monitor_data()
                self._monitor1_data_list.extend(monitor1_data)
            else:
                logging.error("Core type %d is unknown.", core_info.core_type)

    def save(self: any) -> None:
        """
        save parser data to db
        :return: None
        """
        if not self._monitor0_data_list or not self._monitor1_data_list:
            logging.warning("Monitor data list is empty!")
            return

        with self._model as _model:
            self._model.create_table()
            _model.flush(DBNameConstant.TABLE_MONITOR0, self._monitor0_data_list)
            _model.flush(DBNameConstant.TABLE_MONITOR1, self._monitor1_data_list)

    def _file_group_by_core(self: any) -> dict:
        """
        create dict whose key is core id and value is file list
        :return: None
        """
        biu_perf_all_files = self._file_list.get(DataTag.BIU_PERF, [])
        biu_perf_file_dict = {}
        for _file in biu_perf_all_files:
            core_name = _file.split(".")[1]
            core_info = biu_perf_file_dict.setdefault(core_name, CoreInfo(core_name))
            core_info.file_list.append(_file)
        return biu_perf_file_dict
