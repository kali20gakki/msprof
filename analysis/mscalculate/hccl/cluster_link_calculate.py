#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to calculate cluster link from hccl.
Copyright Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
"""

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_multi_process import MsMultiProcess
from model.hccl.hccl_model import HCCLModel


class ClusterLinkCalculator:
    """
    class used to calculate slow link in cluster
    """
    THRESHOLD_VALUE = 0.2

    def __init__(self: any, file_list: list) -> None:
        self._file_list = file_list
        self.slow_link_list = []
        self.result_dict = {}

    def get_cluster_link_data(self: any, link_data: dict, link_type: str) -> None:
        """
        :return:
        """
        max_value = 0
        link_dict = {}
        if link_data.get(link_type, []):
            for value in link_data.get(link_type, []):
                key = "{0}-{1}".format(value[1], value[2])
                max_value = max(max_value, value[3])
                slow_link = "slow rank link {0}, bandwidth is {1}% lower than average.".format(key, max_value)
                link_dict.update({key: slow_link})
            self.result_dict.setdefault(link_type, []).extend(list(link_dict.values()))

    def get_slow_link_dict(self: any) -> None:
        """
        save hccl data into database
        :return: None
        """

        for link_data in self.slow_link_list:
            for link_type in Constant.LINK_TYPE_LIST:
                self.get_cluster_link_data(link_data, link_type)

    def get_single_slow_link_list(self: any) -> None:
        """
        get slow link dict in single project
        """
        for _file_path in self._file_list:
            self.slow_link_list.append(
                ClusterSingleLinkCalculator(_file_path).ms_run())

    def run(self: any) -> dict:
        """
        entrance for calculating ge
        :return: None
        """
        self.get_single_slow_link_list()
        self.get_slow_link_dict()
        return self.result_dict


class ClusterSingleLinkCalculator(MsMultiProcess):
    """
    class used to calculate slow link in PROF
    """
    THRESHOLD_VALUE = 20

    def __init__(self: any, project_path: str) -> None:
        super().__init__({'result_dir': project_path})
        self._project_path = project_path
        self.model = None
        self.hccl_data = []
        self.link_list = []
        self.link_dict = {}
        self.average_data = {}
        self.result_dict = {}

    def calculate_cluster_link_list(self: any, link_type: str) -> list:
        """
        select cluster link data
        :param link_type:
        :return:
        """
        cluster_link_list = []
        try:
            for link_type_data in self.link_dict.get(link_type, []):
                link_bw = round(
                    (self.average_data.get(link_type) - link_type_data[0]) / self.average_data.get(link_type)
                    * NumberConstant.PERCENTAGE,
                    NumberConstant.ROUND_TWO_DECIMAL)
                if link_bw >= self.THRESHOLD_VALUE:
                    link_type_data.extend([link_bw])
                    cluster_link_list.append(link_type_data)
            return cluster_link_list
        except ZeroDivisionError:
            return cluster_link_list

    def get_type_slow_link(self: any, link_type: str) -> None:
        """
        get type link data
        :return: None
        """
        try:
            if self.link_dict.get(link_type, []):
                type_average = sum(float(i[0]) for i in self.link_dict.get(link_type)) \
                               / len(self.link_dict.get(link_type))
                self.average_data.setdefault(link_type, type_average)
                type_list = self.calculate_cluster_link_list(link_type)
                if type_list:
                    self.result_dict.setdefault(link_type, type_list)
        except ZeroDivisionError:
            return

    def get_slow_link_data(self: any) -> None:
        """
        get cluster link data
        :return: None
        """
        for link_type in Constant.LINK_TYPE_LIST:
            self.get_type_slow_link(link_type)

    def get_all_link_dict(self: any) -> None:
        """
        get link dict
        :return: None
        """
        for hccl_data in self.hccl_data:
            if hccl_data.bandwidth == Constant.NULL:
                continue
            if hccl_data.src_rank == hccl_data.dst_rank or hccl_data.src_rank == Constant.ILLEGAL_RANK:
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
            self.get_all_link_dict()
            self.get_slow_link_data()

    def ms_run(self: any) -> dict:
        """
        entrance for calculating cluster link data
        :return: None
        """
        self.calculate()
        return self.result_dict
