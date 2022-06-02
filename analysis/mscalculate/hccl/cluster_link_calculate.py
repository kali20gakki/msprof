#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to calculate cluster link from hccl.
Copyright Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
"""

from multiprocessing import Manager
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_multi_process import MsMultiProcess
from model.hccl.hccl_model import HCCLModel


class ClusterLinkCalculator:
    """
    class used to calculate ge hash info
    """
    THRESHOLD_VALUE = 0.2
    LINK_TYPE_LIST = [Constant.TYPE_SDMA, Constant.TYPE_RDMA]

    def __init__(self: any, file_list: list) -> None:
        self._file_list = file_list
        self.slow_link_list = []
        self.current_dict = {}
        self.result_dict = {}

    def format_link_data(self: any) -> dict:
        """
        save hccl data into database
        :return: None
        """
        for link_type in self.current_dict.keys():
            type_tuning = "{0}: link type and percent(link bandwidth/bandwidth average)".format(link_type)
            self.result_dict.setdefault(type_tuning, self.current_dict.get(link_type))

        return self.result_dict

    def get_slow_dict(self: any) -> None:
        """
        save hccl data into database
        :return: None
        """
        link_dict = {}
        max_value = 0
        for link_data in self.slow_link_list:
            for type_link_data in link_data.items():
                if type_link_data[1]:
                    for value in type_link_data[1]:
                        key = "{0}-{1}".format(value[1], value[2])
                        max_value = max(max_value, value[3])
                        link_dict.update({key: max_value})
                    self.current_dict.setdefault(type_link_data[0], []).extend(list(link_dict.items()))

    def get_single_slow_link_list(self: any) -> None:
        """
        get slow link dict in single project
        """
        parsing_obj = []
        with Manager() as manager:
            data_list = manager.list()
            for _file_path in self._file_list:
                parsing_obj.append(ClusterSingleLinkCalculator(_file_path, {'result_dir': _file_path}, data_list))
            # start parsing processor
            for item in parsing_obj:
                item.start()

            # join parsing processor
            for item in parsing_obj:
                item.join()
            self.slow_link_list = data_list._getvalue()

    def run(self: any) -> dict:
        """
        entrance for calculating ge
        :return: None
        """
        self.get_single_slow_link_list()
        self.get_slow_dict()
        return self.format_link_data()


class ClusterSingleLinkCalculator(MsMultiProcess):
    """
        class used to calculate ge hash info
        """
    THRESHOLD_VALUE = 20

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
        try:
            for link_type_data in self.link_dict.get(link_type, []):
                link_bw = round(
                    abs(link_type_data[0] - self.average_data.get(link_type)) / self.average_data.get(link_type)
                    * NumberConstant.PERCENTAGE,
                    NumberConstant.ROUND_TWO_DECIMAL)
                if link_bw >= self.THRESHOLD_VALUE:
                    link_type_data.extend([link_bw])
                    cluster_link_list.append(link_type_data)
            return cluster_link_list
        except ZeroDivisionError:
            return cluster_link_list

    def get_slow_link_data(self: any) -> None:
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
        if rdma_list:
            self.result_dict.setdefault(Constant.TYPE_RDMA, rdma_list)
        if sdma_list:
            self.result_dict.setdefault(Constant.TYPE_SDMA, sdma_list)

    def get_all_link_dict(self: any) -> None:
        """
        get link dict
        :return: None
        """
        for hccl_data in self.hccl_data:
            if hccl_data.bandwidth == Constant.NULL:
                continue
            if hccl_data.src_rank == hccl_data.dst_rank or hccl_data.src_rank == Constant.GE_OP_MODEL_ID:
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

    def ms_run(self: any) -> None:
        """
        entrance for calculating cluster link data
        :return: None
        """
        self.calculate()
        if self.result_dict:
            self.data_list.append(self.result_dict)
