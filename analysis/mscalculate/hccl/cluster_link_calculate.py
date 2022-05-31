#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to calculate cluster link from hccl.
Copyright Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
"""
import os
from multiprocessing import Manager

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from model.hccl.hccl_model import HCCLModel
from mscalculate.interface.icalculator import ICalculator
from profiling_bean.prof_enum.data_tag import DataTag


class ClusterLinkCalculator(MsMultiProcess):
    """
    class used to calculate ge hash info
    """
    THRESHOLD_VALUE = 0.2
    LINK_TYPE_LIST = [Constant.TYPE_SDMA, Constant.TYPE_RDMA]

    def __init__(self: any, file_list: list, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._file_list = file_list
        self.slow_link_list = []
        self.result_dict = {}

    def get_slow_dict(self: any) -> None:
        """
        save hccl data into database
        :return: None
        """

        for link_data in self.slow_link_list:
            for type_link_data in link_data.items():
                self.result_dict.setdefault(type_link_data[0], []).append(type_link_data[1])

    def get_single_slow_link_list(self: any) -> None:
        """
        get slow link dict in single project
        """
        parsing_obj = []
        with Manager() as manager:
            data_list = manager.list()
            for _file_path in self._file_list:
                parsing_obj.append(ClusterSingleLinkCalculator(_file_path, self._sample_config, data_list))
            # start parsing processor
            for item in parsing_obj:
                item.start()

            # join parsing processor
            for item in parsing_obj:
                item.join()
            self.slow_link_list = data_list.copy()

    def ms_run(self: any) -> None:
        """
        entrance for calculating ge
        :return: None
        """
        self.get_single_slow_link_list()


# class ClusterLinkCalculator(ICalculator, MsMultiProcess):
class ClusterSingleLinkCalculator(MsMultiProcess):
    """
        class used to calculate ge hash info
        """
    THRESHOLD_VALUE = 0.2

    # def __init__(self: any, project_path: dict, sample_config: dict) -> None:
    def __init__(self: any, project_path: str, sample_config: dict, data_list: list) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._project_path = project_path
        self.model = None
        self.hccl_data = []
        self.link_list = []
        self.link_dict = {}
        self.average_data = {}
        self.result_dict = {}
        self.data_list = data_list

    def calculate_cluster_link_list(self: any, link_type: str) -> list:
        """
        select cluster link data
        :param link_type:
        :return:
        """
        cluster_link_list = []
        for link_type_data in self.link_dict.get(link_type, []):
            link_bw = round(
                abs(link_type_data[0] - self.average_data.get(link_type)) / self.average_data.get(link_type),
                NumberConstant.ROUND_TWO_DECIMAL)
            if link_bw >= self.THRESHOLD_VALUE:
                link_type_data.extend([link_bw])
                cluster_link_list.append(link_type_data)
        return cluster_link_list

    def get_cluster_link_data(self: any) -> None:
        """
        get cluster link data
        :return: None
        """
        if self.link_dict.get(Constant.TYPE_RDMA, []):
            rdma_average = sum(float(i[0]) for i in self.link_dict.get(Constant.TYPE_RDMA)) \
                           / len(self.link_dict.get(Constant.TYPE_RDMA))
            self.average_data.setdefault(Constant.TYPE_RDMA, rdma_average)
        if self.link_dict.get(Constant.TYPE_SDMA, []):
            sdma_average = sum(float(i[0]) for i in self.link_dict.get(Constant.TYPE_SDMA)) \
                           / len(self.link_dict.get(Constant.TYPE_SDMA))
            self.average_data.setdefault(Constant.TYPE_SDMA, sdma_average)
        rdma_list = self.calculate_cluster_link_list(Constant.TYPE_RDMA)
        sdma_list = self.calculate_cluster_link_list(Constant.TYPE_SDMA)
        self.result_dict.setdefault(Constant.TYPE_RDMA, rdma_list)
        self.result_dict.setdefault(Constant.TYPE_SDMA, sdma_list)

    def get_cluster_link_dict(self: any) -> None:
        """
        update ge data with hash data
        :return: None
        """
        for hccl_data in self.hccl_data:
            if hccl_data.bandwidth == Constant.NULL:
                continue
            if hccl_data.src_rank == hccl_data.dst_rank:
                continue
            if hccl_data.transport_type == Constant.TYPE_RDMA or hccl_data.transport_type == Constant.TYPE_SDMA:
                self.link_dict.setdefault(hccl_data.transport_type, []). \
                    append([float(hccl_data.bandwidth), hccl_data.src_rank, hccl_data.dst_rank])

    def calculate(self: any) -> None:
        """
        calculate hccl data
        :return: None
        """
        with HCCLModel(self._project_path, [DBNameConstant.TABLE_HCCL_ALL_REDUCE]) as self.model:
            if not self.model.check_table():
                return
            self.hccl_data = self.model.get_hccl_data()
            if not self.hccl_data:
                return
            self.get_cluster_link_dict()
            self.get_cluster_link_data()

    def save(self: any) -> None:
        """
        save hccl data into database
        :return: None
        """
        pass

    def ms_run(self: any) -> None:
        """
        entrance for calculating ge
        :return: None
        """
        self.calculate()
        if self.result_dict:
            self.data_list.append(self.result_dict)
