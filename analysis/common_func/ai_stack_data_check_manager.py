#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.

from common_func import file_name_manager
from common_func.config_mgr import ConfigMgr
from common_func.data_check_manager import DataCheckManager
from common_func.db_name_constant import DBNameConstant
from common_func.msvp_common import path_check
from common_func.path_manager import PathManager
from common_func.db_manager import DBManager


class AiStackDataCheckManager(DataCheckManager):
    """
    The ai stack data check manager
    """

    @classmethod
    def contain_acl_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain acl data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_acl_compiles(),
                                    device_id=device_id)

    @classmethod
    def contain_runtime_api_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain runtime.api. data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_runtime_api_compiles(), device_id=device_id) or \
               DBManager.check_tables_in_db(
                   PathManager.get_db_path(result_dir, DBNameConstant.DB_RUNTIME), DBNameConstant.TABLE_API_CALL)

    @classmethod
    def contain_ge_model_load_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain Framework.model_load_info data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ge_model_load_compiles(),
                                    device_id=device_id)

    @classmethod
    def contain_ge_fusion_op_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain Framework.model_load_info data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ge_fusion_op_compiles(),
                                    device_id=device_id)

    @classmethod
    def contain_fusion_op_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain Framework.model_load_info data or not
        """
        return cls.contain_ge_model_load_data(result_dir, device_id=device_id) and \
               cls.contain_ge_fusion_op_data(result_dir, device_id=device_id)

    @classmethod
    def contain_ge_model_time_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain Framework.model_time_info data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ge_model_time_compiles(),
                                    device_id=device_id)

    @classmethod
    def contain_ge_op_execute_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain Framework.dynamic_op_execute data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ge_host_compiles(), device_id=device_id)

    @classmethod
    def contain_ge_step_info_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain Framework.step_info data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ge_step_info_compiles(),
                                    device_id=device_id)

    @classmethod
    def contain_l2_cache_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain l2_cache data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_l2_cache_compiles(),
                                    device_id=device_id)

    @classmethod
    def contain_ai_core_sample_based(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain ai core data or not
        """
        return bool(path_check(PathManager.get_db_path(result_dir, 'aicore_{}.db'.format(str(device_id))))) or \
               (cls.check_data_exist(result_dir, file_name_manager.get_ai_core_compiles(),
                                     device_id=device_id) and ConfigMgr.is_ai_core_sample_based(result_dir))

    @classmethod
    def contain_aiv_core_sample_based(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain ai core data or not
        """
        return bool(path_check(PathManager.get_db_path(result_dir, 'ai_vector_core_{}.db'.format(str(device_id))))) or \
               cls.check_data_exist(result_dir, file_name_manager.get_aiv_compiles(),
                                    device_id=device_id) and ConfigMgr.is_aiv_sample_based(result_dir)

    @classmethod
    def contain_training_trace_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain training trace data or not
        """
        return True if path_check(PathManager.get_db_path(result_dir, DBNameConstant.DB_TRACE)) else False

    @classmethod
    def contain_dp_aicpu_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain aicpu data or not
        """
        return cls.check_data_exist(result_dir,
                                    file_name_manager.get_data_preprocess_compiles('AICPU'),
                                    device_id=device_id)

    @classmethod
    def contain_data_preprocess_dp_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain dp data or not
        """
        return cls.check_data_exist(result_dir,
                                    file_name_manager.get_data_preprocess_compiles('DP'),
                                    device_id=device_id)

    @classmethod
    def contain_task_time_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain step_trace data or not
        """
        return cls._contain_ts_track_data(result_dir, device_id=device_id) or \
               cls._contain_hwts_data(result_dir, device_id=device_id) or \
               cls.contain_stars_soc_data(result_dir, device_id=device_id) or \
               (cls._contain_hwts_aiv_data(result_dir, device_id=device_id) or
                cls._contain_ts_track_aiv_data(result_dir, device_id=device_id))

    @classmethod
    def contain_task_time_task(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain step_trace data or not
        """
        return cls.contain_task_time_data(result_dir, device_id) or \
               AiStackDataCheckManager.contain_stars_soc_data(result_dir, device_id)

    @classmethod
    def contain_op_summary_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain op summary data or not
        """
        return cls.contain_op_static_data(result_dir, device_id=device_id) or \
               cls.contain_op_summary_without_ge_data(result_dir, device_id=device_id)

    @classmethod
    def contain_op_summary_without_ge_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain op summary data or not
        """
        return not cls._contain_ge_task_data(result_dir, device_id=device_id) and \
               cls.contain_task_time_data(result_dir, device_id=device_id)

    @classmethod
    def contain_op_static_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain op summary data or not
        """
        return cls._contain_ge_task_data(result_dir, device_id=device_id) and \
               cls.contain_task_time_data(result_dir, device_id=device_id)

    @classmethod
    def contain_ai_stack_time_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain acl data or not
        """
        return cls.contain_acl_data(result_dir, device_id=device_id) or \
               cls.contain_ge_model_time_data(result_dir, device_id=device_id) or \
               cls.contain_runtime_api_data(result_dir, device_id=device_id) or \
               cls.contain_task_time_data(result_dir, device_id=device_id)

    @classmethod
    def contain_core_cpu_reduce_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain step_trace data or not
        """
        return cls.contain_task_time_data(result_dir, device_id=device_id) or \
               cls.contain_training_trace_data(result_dir)

    @classmethod
    def contain_stars_soc_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain stars soc log data or not
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained stars soc data, true or false
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_soc_log_compiles(),
                                    device_id=device_id)

    @classmethod
    def contain_hccl_hcom_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain hccl hcom data or not
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained hccl hcom data, true or false
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_hccl_hcom_compiles(),
                                    device_id=device_id)

    @classmethod
    def contain_training_trace_data_or_step(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain training trace data or step trace data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained training trace data or step trace data, true or false
        """
        return cls.contain_training_trace_data(result_dir, device_id=device_id)

    @classmethod
    def contain_msproftx_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain msproftx data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained msproftx data, true or false
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_msproftx_summary_timeline_compiles(),
                                    device_id=device_id)

    @classmethod
    def contain_stars_soc_profiler_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain stars soc profile data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained msproftx data, true or false
        """
        return cls._contain_stars_profiler_data(result_dir, device_id=device_id) \
               and path_check(PathManager.get_db_path(result_dir, DBNameConstant.DB_STARS_SOC))

    @classmethod
    def contain_stars_chip_trans_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain msproftx data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained msproftx data, true or false
        """
        return cls._contain_stars_profiler_data(result_dir, device_id=device_id) \
               and path_check(PathManager.get_db_path(result_dir, DBNameConstant.DB_STARS_CHIP_TRANS))

    @classmethod
    def contain_acc_pmu_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain accpmu data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained accpmu data, true or false
        """
        return cls._contain_stars_profiler_data(result_dir, device_id=device_id) \
               and bool(path_check(PathManager.get_db_path(result_dir, DBNameConstant.DB_ACC_PMU)))

    @classmethod
    def contain_thread_group_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain acl,ge,ge_op_execute or runtime api group by thread
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained data group by thread, true or false
        """
        return cls.contain_acl_data(result_dir, device_id=device_id) or \
               cls.contain_ge_model_time_data(result_dir, device_id=device_id) or \
               cls.contain_ge_op_execute_data(result_dir, device_id=device_id) or \
               cls.contain_runtime_api_data(result_dir, device_id=device_id)

    @classmethod
    def contain_stars_low_power_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain lowpower sample data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained lowpower sample data, true or false
        """
        return cls._contain_stars_profiler_data(result_dir, device_id) \
               and path_check(PathManager.get_db_path(result_dir, DBNameConstant.DB_LOW_POWER))

    @classmethod
    def contain_biu_perf_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain biu perf data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained biu data, true or false
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_biu_compiles(),
                                    device_id=device_id)

    @classmethod
    def _contain_stars_profiler_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain stars_soc_profiler data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_soc_profiler_compiles(),
                                    device_id=device_id)

    @classmethod
    def _contain_ge_task_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain Framework.task_desc_info data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ge_task_compiles(),
                                    device_id=device_id)

    @classmethod
    def _contain_hwts_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain hwts_log data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_hwts_compiles(),
                                    device_id=device_id)

    @classmethod
    def _contain_hwts_aiv_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain hwts_log data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_hwts_vector_compiles(),
                                    device_id=device_id)

    @classmethod
    def _contain_ts_track_aiv_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain step_trace data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ts_track_aiv_compiles(),
                                    device_id=device_id)

    @classmethod
    def _contain_ts_track_data(cls: any, result_dir: str, device_id: any = None) -> bool:
        """
        The data path contain step_trace data or not
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_ts_track_compiles(),
                                    device_id=device_id)

    @classmethod
    def contain_pytorch_operator_profiler_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain stars msproftx torch data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained msproftx torch data, true or false
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_msproftx_torch_compiles(),
                                    device_id=device_id)

    @classmethod
    def contain_task_queue_data(cls: any, result_dir: str, device_id: int = None) -> bool:
        """
        The data path contain msproftx pipeline data
        :param result_dir: data dir path
        :param device_id: device id
        :return: if contained msproftx pipeline data, true or false
        """
        return cls.check_data_exist(result_dir, file_name_manager.get_msproftx_pipeline_compiles(),
                                    device_id=device_id)
