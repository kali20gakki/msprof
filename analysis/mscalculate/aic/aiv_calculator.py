# coding=utf-8
"""
function: script used to parse ffts pmu data and save it to db
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""
from common_func.config_mgr import ConfigMgr
from common_func.ms_multi_process import MsMultiProcess
from common_func.utils import Utils
from model.aic.aiv_pmu_model import AivPmuModel
from mscalculate.aic.aic_calculator import AicCalculator
from mscalculate.aic.aic_utils import AicPmuUtils
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.aiv_pmu import AivPmuBean


class AivCalculator(AicCalculator, MsMultiProcess):
    """
    class used to parse aicore data by iter
    """
    AICORE_LOG_SIZE = 128

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(file_list, sample_config)
        self._file_list = file_list.get(DataTag.AIV, [])
        self._aiv_data_list = []
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    def aiv_calculate(self: any) -> None:
        """
        calculate for ai vector core
        :return: None
        """
        self._parse_all_file()

    def _parse(self: any, all_log_bytes: bytes) -> None:
        aic_pmu_events = AicPmuUtils.get_pmu_events(
            ConfigMgr.read_sample_config(self._project_path).get('aiv_profiling_events'))
        for log_data in Utils.chunks(all_log_bytes, self.AICORE_LOG_SIZE):
            _aic_pmu_log = AivPmuBean.decode(log_data)
            self.calculate_pmu_list(_aic_pmu_log, aic_pmu_events, self._aiv_data_list)

    def save(self: any) -> None:
        """
        :return:
        """
        if self._aiv_data_list:
            aiv_pmu_model = AivPmuModel(self._project_path)
            aiv_pmu_model.init()
            aiv_pmu_model.flush(self._aiv_data_list)
            aiv_pmu_model.finalize()

    def ms_run(self: any) -> None:
        """

        :return:
        """
        if self._file_list:
            self.aiv_calculate()
            self.save()
