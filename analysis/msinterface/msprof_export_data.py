#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import json
import threading

from config.config_manager import ConfigManager
from common_func.config_mgr import ConfigMgr
from common_func.data_manager import DataManager
from common_func.db_name_constant import DBNameConstant
from common_func.empty_class import EmptyClass
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.msvp_constant import MsvpConstant
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.utils import Utils
from host_prof.host_prof_presenter_manager import HostExportType
from host_prof.host_prof_presenter_manager import get_host_prof_timeline
from host_prof.host_syscall.presenter.host_syscall_presenter import HostSyscallPresenter
from msinterface.msprof_data_storage import MsprofDataStorage
from msinterface.msprof_timeline import MsprofTimeline
from msparser.aicpu.parse_dp_data import ParseDpData
from viewer.acl.acl_viewer import AclViewer
from viewer.ai_core_op_report import AiCoreOpReport
from viewer.ai_core_op_report import ReportOPCounter
from viewer.ai_core_report import get_core_sample_data
from viewer.aicpu_viewer import ParseAiCpuData
from viewer.biu_perf_viewer import BiuPerfViewer
from viewer.cpu_data_report import get_aictrl_pmu_events
from viewer.cpu_data_report import get_cpu_hot_function
from viewer.cpu_data_report import get_ts_pmu_events
from viewer.cpu_usage_report import get_process_cpu_usage
from viewer.cpu_usage_report import get_sys_cpu_usage_data
from viewer.ge.ge_op_execute_viewer import GeOpExecuteViewer
from viewer.ge_info_report import get_ge_model_data
from viewer.get_hccl_export_data import HCCLExport
from viewer.get_l2_cache_data import add_op_name, get_l2_cache_sample_data
from viewer.get_l2_cache_data import get_l2_cache_data
from viewer.get_msvp_llc_timeline_training import get_ddr_timeline
from viewer.get_msvp_llc_timeline_training import get_hbm_timeline
from viewer.get_msvp_llc_timeline_training import get_llc_timeline
from viewer.get_msvp_summary_mem import get_process_mem_data
from viewer.get_msvp_summary_mem import get_sys_mem_data
from viewer.get_msvp_summary_training import get_hbm_summary_data
from viewer.get_trace_timeline import get_aicore_utilization_timeline
from viewer.get_trace_timeline import get_dvpp_timeline
from viewer.get_trace_timeline import get_ge_timeline_data
from viewer.get_trace_timeline import get_hccs_timeline
from viewer.get_trace_timeline import get_network_timeline
from viewer.get_trace_timeline import get_pcie_timeline
from viewer.hardware_info_report import get_ddr_data
from viewer.hardware_info_report import get_llc_bandwidth
from viewer.hardware_info_report import llc_capacity_data
from viewer.hardware_info_view import get_llc_train_summary
from viewer.interconnection_view import InterConnectionView
from viewer.msproftx_viewer import MsprofTxViewer
from viewer.peripheral_report import get_peripheral_dvpp_data
from viewer.peripheral_report import get_peripheral_nic_data
from viewer.runtime.runtime_api_viewer import RuntimeApiViewer
from viewer.runtime_report import get_task_scheduler_data
from viewer.stars.acc_pmu_viewer import AccPmuViewer
from viewer.stars.acsq_task_viewer import AcsqTaskViewer
from viewer.stars.ffts_log_viewer import FftsLogViewer
from viewer.stars.low_power_viewer import LowPowerViewer
from viewer.stars.stars_chip_trans_view import StarsChipTransView
from viewer.stars.stars_soc_view import StarsSocView
from viewer.thread_group_viewer import ThreadGroupViewer
from viewer.top_down_report import TopDownData
from viewer.training.core_cpu_reduce_viewer import CoreCpuReduceViewer
from viewer.training.step_trace_viewer import StepTraceViewer
from viewer.training.task_op_viewer import TaskOpViewer
from viewer.ts_cpu_report import TsCpuReport


class MsProfExportDataUtils:
    """
    Export data such as csv or json
    """
    cfg_parser = None
    init_cfg_finished = False
    LOCK = threading.Lock()

    @staticmethod
    def _get_runtime_api_data(configs: dict, params: dict) -> any:
        """
        get runtime data handler
        """
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return RuntimeApiViewer(configs, params).get_timeline_data()
        return RuntimeApiViewer(configs, params).get_summary_data()

    @staticmethod
    def _get_task_time_data(configs: dict, params: dict) -> any:
        """
        get task scheduler data
        """
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            message = {
                "job_id": params.get(StrConstant.PARAM_JOB_ID),
                "host_id": MsProfCommonConstant.DEFAULT_IP,
                "device_id": params.get(StrConstant.PARAM_DEVICE_ID),
                "iter_id": params.get(StrConstant.PARAM_ITER_ID),
                "result_dir": params.get(StrConstant.PARAM_RESULT_DIR),
                "model_id": params.get(StrConstant.PARAM_MODEL_ID)
            }
            return CoreCpuReduceViewer.get_task_time_data(message)
        if ChipManager().is_chip_v1():
            db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR),
                                              configs.get(StrConstant.CONFIG_DB))
            return get_task_scheduler_data(db_path, configs.get(StrConstant.CONFIG_TABLE), configs,
                                           params)
        message = {
            'job_id': params.get(StrConstant.PARAM_JOB_ID),
            'host_id': MsProfCommonConstant.DEFAULT_IP,
            'device_id': params.get(StrConstant.PARAM_DEVICE_ID),
            'result_dir': params.get(StrConstant.PARAM_RESULT_DIR)
        }
        return TaskOpViewer.get_task_op_summary(message)

    @staticmethod
    def _get_ddr_data(configs: dict, params: dict) -> any:
        """
        get ddr data
        """
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            ddr_param = {
                'project_path': params.get(StrConstant.PARAM_RESULT_DIR),
                'device_id': params.get(StrConstant.PARAM_DEVICE_ID),
                'start_time': NumberConstant.DEFAULT_START_TIME,
                'end_time': NumberConstant.DEFAULT_END_TIME
            }
            result = get_ddr_timeline(ddr_param)
            return result

        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR),
                                          configs.get(StrConstant.CONFIG_DB).format(
                                              params.get(StrConstant.PARAM_DEVICE_ID)))
        return get_ddr_data(db_path, params.get(StrConstant.PARAM_DEVICE_ID), configs)

    @staticmethod
    def _get_cpu_usage_data(configs: dict, params: dict) -> any:
        """
        get system cpu usage data
        """
        db_name = configs.get(StrConstant.CONFIG_DB).format(params.get(StrConstant.PARAM_DEVICE_ID))
        if InfoConfReader().is_host_profiling():
            db_name = DBNameConstant.DB_HOST_SYS_USAGE_CPU
        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR), db_name)
        if params.get(StrConstant.DATA_TYPE) == "cpu_usage":
            return get_sys_cpu_usage_data(db_path, configs.get(StrConstant.CONFIG_TABLE), configs)
        if params.get(StrConstant.DATA_TYPE) == "process_cpu_usage":
            return get_process_cpu_usage(db_path, configs.get(StrConstant.CONFIG_TABLE), configs)
        return MsvpConstant.MSVP_EMPTY_DATA

    @staticmethod
    def _get_cpu_top_funcs(configs: dict, params: dict) -> any:
        """
        get cpu pmu events
        """
        return get_cpu_hot_function(params.get(StrConstant.PARAM_RESULT_DIR),
                                    configs.get(StrConstant.CONFIG_DB).format(params.get(StrConstant.PARAM_DEVICE_ID)),
                                    configs.get(StrConstant.CONFIG_TABLE), configs.get(StrConstant.CONFIG_HEADERS))

    @staticmethod
    def _get_ts_cpu_top_funcs(configs: dict, params: dict) -> any:
        """
        get ts cpu top function
        """
        return TsCpuReport().get_output_top_function(
            configs.get(StrConstant.CONFIG_DB).format(params.get(StrConstant.PARAM_DEVICE_ID)),
            params.get(StrConstant.PARAM_RESULT_DIR))

    @staticmethod
    def _get_cpu_pmu_events(configs: dict, params: dict) -> any:
        """
        get cpu pmu events
        """
        if params.get(StrConstant.PARAM_DATA_TYPE) == StrConstant.CTL_CPU_PMU or params.get(
                StrConstant.PARAM_DATA_TYPE) == StrConstant.AI_CPU_PMU:
            return get_aictrl_pmu_events(params.get(StrConstant.PARAM_RESULT_DIR),
                                         configs.get(StrConstant.CONFIG_DB).format(
                                             params.get(StrConstant.PARAM_DEVICE_ID)),
                                         configs.get(StrConstant.CONFIG_TABLE), configs.get(StrConstant.CONFIG_HEADERS))
        if params.get(StrConstant.PARAM_DATA_TYPE) == StrConstant.TS_CPU_PMU:
            return get_ts_pmu_events(params.get(StrConstant.PARAM_RESULT_DIR),
                                     configs.get(StrConstant.CONFIG_DB).format(params.get(StrConstant.PARAM_DEVICE_ID)),
                                     configs.get(StrConstant.CONFIG_TABLE), configs.get(StrConstant.CONFIG_HEADERS))
        return MsvpConstant.MSVP_EMPTY_DATA

    @staticmethod
    def _get_memory_data(configs: dict, params: dict) -> any:
        """
        get memory data
        """
        db_name = configs.get(StrConstant.CONFIG_DB).format(params.get(StrConstant.PARAM_DEVICE_ID))
        if InfoConfReader().is_host_profiling():
            db_name = DBNameConstant.DB_HOST_SYS_USAGE_MEM
        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR), db_name)
        if params.get(StrConstant.PARAM_DATA_TYPE) == "sys_mem":
            return get_sys_mem_data(db_path, configs.get(StrConstant.CONFIG_TABLE), configs)
        if params.get(StrConstant.PARAM_DATA_TYPE) == "process_mem":
            return get_process_mem_data(db_path, configs.get(StrConstant.CONFIG_TABLE), configs)
        return MsvpConstant.MSVP_EMPTY_DATA

    @staticmethod
    def _get_acl_data(configs: dict, params: dict) -> any:
        """
        get acl data
        """
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return AclViewer(configs, params).get_timeline_data()
        return AclViewer(configs, params).get_summary_data()

    @staticmethod
    def _get_acl_statistic_data(configs: dict, params: dict) -> any:
        """
        get acl data
        """
        return AclViewer(configs, params).get_acl_statistic_data()

    @staticmethod
    def _get_op_summary_data(configs: dict, params: dict) -> any:
        """
        get ai core op summary detail table
        """
        db_name = DBNameConstant.DB_AICORE_OP_SUMMARY
        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR), db_name)
        return AiCoreOpReport.get_op_summary_data(params.get(StrConstant.PARAM_RESULT_DIR),
                                                  db_path, params.get(StrConstant.PARAM_ITER_ID), configs)

    @staticmethod
    def _get_ai_stack_time_data(configs: dict, params: dict) -> any:
        """
        get ai stack time data
        """
        _ = configs
        device_id = params.get(StrConstant.PARAM_DEVICE_ID)
        index_id = params.get(StrConstant.PARAM_ITER_ID)
        result_dir = params.get(StrConstant.PARAM_RESULT_DIR)

        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return TopDownData.get_top_down_timeline_data(
                result_dir, device_id, index_id)

        return TopDownData.get_top_down_data(result_dir, device_id, index_id)

    @staticmethod
    def _get_l2_cache_data(configs: dict, params: dict) -> tuple:
        """
        get l2 cache data
        """
        db_name = configs.get(StrConstant.CONFIG_DB)
        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR), db_name)
        if ChipManager().is_chip_v4():
            return get_l2_cache_sample_data(
                db_path, DBNameConstant.TABLE_L2CACHE_SAMPLE, configs.get(StrConstant.CONFIG_HEADERS))
        headers, data, count = get_l2_cache_data(
            db_path, configs.get(StrConstant.CONFIG_TABLE), params.get(StrConstant.PARAM_DEVICE_ID),
            configs.get(StrConstant.CONFIG_UNUSED_COLS))
        if headers and data:
            op_dict = DataManager.get_op_dict(params.get(StrConstant.PARAM_RESULT_DIR))
            if op_dict:
                if add_op_name(headers, data, op_dict):
                    headers.append("Op Name")
        return headers, data, count

    @staticmethod
    def _get_step_trace_data(configs: dict, params: dict) -> any:
        """
        get training trace data for desired job ID
        """
        message = {
            "job_id": params.get(StrConstant.PARAM_JOB_ID),
            "device_id": params.get(StrConstant.PARAM_DEVICE_ID),
            'host_id': MsProfCommonConstant.DEFAULT_IP,
            'project_path': params.get(StrConstant.PARAM_RESULT_DIR)
        }
        try:
            if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
                return StepTraceViewer.get_step_trace_timeline(message)
            else:
                reduce_headers, data, count = StepTraceViewer \
                    .get_step_trace_summary(message)
                headers = configs[StrConstant.CONFIG_HEADERS] + reduce_headers
                return headers, data, count
        except (OSError, SystemError, ValueError, TypeError,
                RuntimeError) as err:
            return json.dumps({
                "data": "", "status": NumberConstant.ERROR,
                "info": "message error: {}".format(err)
            })

    @staticmethod
    def _get_op_statistic_data(configs: dict, params: dict) -> any:
        """
        get op statistic data
        """
        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR), configs.get(StrConstant.CONFIG_DB))
        return ReportOPCounter.report_op(db_path,
                                         configs.get(StrConstant.CONFIG_HEADERS))

    @staticmethod
    def _get_dvpp_data(configs: dict, params: dict) -> any:
        """
        get dvpp data
        """
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            dvpp_param = {
                'project_path': params.get(StrConstant.PARAM_RESULT_DIR),
                'device_id': params.get(StrConstant.PARAM_DEVICE_ID),
                'dvppid': 'all',
                'start_time': NumberConstant.DEFAULT_START_TIME,
                'end_time': NumberConstant.DEFAULT_END_TIME
            }
            result = get_dvpp_timeline(dvpp_param)
            return result

        data_type = params.get(StrConstant.PARAM_DATA_TYPE)
        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR),
                                          configs.get(StrConstant.CONFIG_DB).format(
                                              params.get(StrConstant.PARAM_DEVICE_ID)))
        if data_type == StrConstant.DVPP_DATA:
            return get_peripheral_dvpp_data(db_path, configs.get(StrConstant.CONFIG_TABLE),
                                            params.get(StrConstant.PARAM_DEVICE_ID), configs)
        return MsvpConstant.MSVP_EMPTY_DATA

    @staticmethod
    def _get_nic_data(configs: dict, params: dict) -> any:
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            nic_result = get_network_timeline(params.get(StrConstant.PARAM_RESULT_DIR),
                                              params.get(StrConstant.PARAM_DEVICE_ID),
                                              NumberConstant.DEFAULT_START_TIME,
                                              NumberConstant.DEFAULT_END_TIME, params.get(StrConstant.DATA_TYPE))
            return nic_result

        data_type = params.get(StrConstant.PARAM_DATA_TYPE)
        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR),
                                          configs.get(StrConstant.CONFIG_DB).format(
                                              params.get(StrConstant.PARAM_DEVICE_ID)))
        if data_type == StrConstant.NIC_DATA:
            return get_peripheral_nic_data(db_path, configs.get(StrConstant.CONFIG_TABLE),
                                           params.get(StrConstant.PARAM_DEVICE_ID), configs)
        return MsvpConstant.MSVP_EMPTY_DATA

    @staticmethod
    def _get_roce_data(configs: dict, params: dict) -> any:
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            roce_result = get_network_timeline(params.get(StrConstant.PARAM_RESULT_DIR),
                                               params.get(StrConstant.PARAM_DEVICE_ID),
                                               NumberConstant.DEFAULT_START_TIME,
                                               NumberConstant.DEFAULT_END_TIME, params.get(StrConstant.DATA_TYPE))
            return roce_result

        data_type = params.get(StrConstant.PARAM_DATA_TYPE)
        db_path = PathManager.get_db_path(params.get(StrConstant.PARAM_RESULT_DIR),
                                          configs.get(StrConstant.CONFIG_DB).format(
                                              params.get(StrConstant.PARAM_DEVICE_ID)))
        if data_type == StrConstant.ROCE_DATA:
            return get_peripheral_nic_data(db_path, configs.get(StrConstant.CONFIG_TABLE),
                                           params.get(StrConstant.PARAM_DEVICE_ID), configs)
        return MsvpConstant.MSVP_EMPTY_DATA

    @staticmethod
    def _get_hccs_data(configs: dict, params: dict) -> any:
        _ = configs
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            result = get_hccs_timeline(params.get(StrConstant.PARAM_RESULT_DIR),
                                       params.get(StrConstant.PARAM_DEVICE_ID),
                                       NumberConstant.DEFAULT_START_TIME,
                                       NumberConstant.DEFAULT_END_TIME)
            return result

        hccs_view = InterConnectionView(params.get(StrConstant.PARAM_RESULT_DIR), {})
        return hccs_view.get_hccs_data(params.get(StrConstant.PARAM_DEVICE_ID))

    @staticmethod
    def __get_mini_llc_data(sample_config: dict, params: dict) -> any:
        """
        get mini llc data, contains bandwidth and capacity
        """
        if sample_config.get(StrConstant.LLC_PROF) == StrConstant.LLC_BAND_ITEM:
            return get_llc_bandwidth(params.get(StrConstant.PARAM_RESULT_DIR),
                                     params.get(StrConstant.PARAM_DEVICE_ID))
        if not sample_config.get(StrConstant.LLC_PROF) == StrConstant.LLC_CAPACITY_ITEM:
            return MsvpConstant.MSVP_EMPTY_DATA
        if not (params.get(StrConstant.DATA_TYPE) == StrConstant.LLC_AICPU or params.get(
                StrConstant.DATA_TYPE) == StrConstant.LLC_CTRLCPU):
            return MsvpConstant.MSVP_EMPTY_DATA
        types = StrConstant.AICPU
        if params.get(StrConstant.DATA_TYPE) != StrConstant.LLC_AICPU:
            types = StrConstant.CTRL_CPU
        return llc_capacity_data(params.get(StrConstant.PARAM_RESULT_DIR),
                                 params.get(StrConstant.PARAM_DEVICE_ID),
                                 types)

    @staticmethod
    def __get_non_mini_llc_data(sample_config: dict, params: dict) -> any:
        """
        get non mini llc data, contains read and write
        """
        result = get_llc_train_summary(params.get(StrConstant.PARAM_RESULT_DIR), sample_config,
                                       params.get(StrConstant.PARAM_DEVICE_ID))
        res = json.loads(result)
        if res['status'] == NumberConstant.SUCCESS:
            llc_data = res['data']['table']
            llc_header = ['Mode', 'Task', 'Hit Rate(%)', 'Throughput(MB/s)']
            llc_data = [[item[key] for key in llc_header] for item in llc_data]
            return llc_header, llc_data, len(llc_data)
        return [], json.dumps({
            "status": NumberConstant.ERROR,
            "info": "Failed to get LLC data(%s) in non-mini scene." % (params.get(StrConstant.DATA_TYPE))
        }), 0

    @staticmethod
    def _get_llc_data(configs: dict, params: dict) -> any:
        _ = configs
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return get_llc_timeline(params)
        sample_config = ConfigMgr.read_sample_config(params.get(StrConstant.PARAM_RESULT_DIR))
        if sample_config:
            if ChipManager().is_chip_v1():
                return MsProfExportDataUtils.__get_mini_llc_data(sample_config, params)
            return MsProfExportDataUtils.__get_non_mini_llc_data(sample_config, params)
        return [], json.dumps({
            "status": NumberConstant.ERROR,
            "info": "Failed to get LLC sample config data(%s)." % (params.get(StrConstant.DATA_TYPE))
        }), 0

    @staticmethod
    def _get_hbm_data(configs: dict, params: dict) -> any:
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            hbm_param = {
                'project_path': params.get(StrConstant.PARAM_RESULT_DIR),
                'device_id': params.get(StrConstant.PARAM_DEVICE_ID),
                'hbm_id': 0,
                'start_time': NumberConstant.DEFAULT_START_TIME,
                'end_time': NumberConstant.DEFAULT_END_TIME
            }
            result = get_hbm_timeline(hbm_param)
            return result
        data = get_hbm_summary_data(params.get(StrConstant.PARAM_RESULT_DIR),
                                    params.get(StrConstant.PARAM_DEVICE_ID))
        if data:
            if isinstance(data, list):
                return configs.get(StrConstant.CONFIG_HEADERS), data, len(data)
            return MsvpConstant.MSVP_EMPTY_DATA
        return MsvpConstant.MSVP_EMPTY_DATA

    @staticmethod
    def _get_pcie_data(configs: dict, params: dict) -> any:
        _ = configs
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            pcie_param = {
                'project_path': params.get(StrConstant.PARAM_RESULT_DIR),
                'device_id': params.get(StrConstant.PARAM_DEVICE_ID),
                'start_time': NumberConstant.DEFAULT_START_TIME,
                'end_time': NumberConstant.DEFAULT_END_TIME
            }
            result = get_pcie_timeline(pcie_param)
            return result
        return InterConnectionView(params.get(StrConstant.PARAM_RESULT_DIR),
                                   {}).get_pcie_summary_data()

    @staticmethod
    def _get_aicpu_data(configs: dict, params: dict) -> any:
        _ = configs
        aicpu_headers, aicpu_data = ParseAiCpuData.analysis_aicpu(params.get(StrConstant.PARAM_RESULT_DIR),
                                                                  params.get(StrConstant.PARAM_ITER_ID),
                                                                  params.get(StrConstant.PARAM_MODEL_ID))
        return aicpu_headers, aicpu_data, len(aicpu_data)

    @staticmethod
    def _get_dp_data(configs: dict, params: dict) -> any:
        _ = configs
        result_dir = params.get(StrConstant.PARAM_RESULT_DIR)
        device_id = params.get(StrConstant.PARAM_DEVICE_ID)

        dp_result = ParseDpData.analyse_dp(
            PathManager.get_data_dir(result_dir), device_id)
        dp_headers = configs.get('headers')
        if dp_result:
            return dp_headers, dp_result, len(dp_result)
        return MsvpConstant.MSVP_EMPTY_DATA

    @staticmethod
    def _get_fusion_op_data(configs: dict, params: dict) -> any:
        return get_ge_model_data(params, configs.get(StrConstant.CONFIG_TABLE), configs)

    @staticmethod
    def _get_host_cpu_usage_data(configs: dict, params: dict) -> any:
        _ = configs
        return get_host_prof_timeline(params.get(StrConstant.PARAM_RESULT_DIR), HostExportType.CPU_USAGE)

    @staticmethod
    def _get_host_mem_usage_data(configs: dict, params: dict) -> any:
        _ = configs
        return get_host_prof_timeline(params.get(StrConstant.PARAM_RESULT_DIR), HostExportType.MEM_USAGE)

    @staticmethod
    def _get_host_network_usage_data(configs: dict, params: dict) -> any:
        _ = configs
        return get_host_prof_timeline(params.get(StrConstant.PARAM_RESULT_DIR), HostExportType.NETWORK_USAGE)

    @staticmethod
    def _get_host_disk_usage_data(configs: dict, params: dict) -> any:
        _ = configs
        return get_host_prof_timeline(params.get(StrConstant.PARAM_RESULT_DIR), HostExportType.DISK_USAGE)

    @staticmethod
    def _get_host_runtime_api(configs: dict, params: dict) -> any:
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.SUMMARY:
            data = HostSyscallPresenter.get_summary_api_info(params.get(StrConstant.PARAM_RESULT_DIR))
            if data:
                return configs.get(StrConstant.CONFIG_HEADERS), data, len(data)
            return MsvpConstant.MSVP_EMPTY_DATA
        return get_host_prof_timeline(params.get(StrConstant.PARAM_RESULT_DIR),
                                      HostExportType.HOST_RUNTIME_API)

    @staticmethod
    def _get_ge_data(configs: dict, params: dict) -> any:
        _ = configs
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return get_ge_timeline_data(params.get(StrConstant.PARAM_RESULT_DIR))
        return json.dumps({
            "status": NumberConstant.WARN,
            "info": "Please check params, "
                    "Currently ge data does not support exporting files other than timeline."
        })

    @staticmethod
    def _get_ge_op_execute_data(configs: dict, params: dict) -> any:
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return GeOpExecuteViewer(configs, params).get_timeline_data()
        return GeOpExecuteViewer(configs, params).get_summary_data()

    @staticmethod
    def _get_bulk_data(configs: dict, params: dict) -> any:
        _ = configs
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return MsprofTimeline().export_all_data()
        return json.dumps({
            "status": NumberConstant.WARN,
            "info": "Please check params, "
                    "Currently bulk data export params should be timeline."
        })

    @staticmethod
    def _get_acsq_task_statistic(configs: dict, params: dict) -> tuple:
        return AcsqTaskViewer(configs, params).get_summary_data()

    @staticmethod
    def _get_sub_task_time(configs: dict, params: dict) -> str:
        """
        get ffts task time data
        """
        return FftsLogViewer(configs, params).get_timeline_data()

    @staticmethod
    def _get_hccl_timeline(configs: dict, params: dict) -> str:
        """
        get hccl timeline data
        :param configs:
        :param params:
        :return:
        """
        _ = configs
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            return HCCLExport(params).get_hccl_timeline_data()
        return json.dumps({
            "status": NumberConstant.WARN,
            "info": "Please check params, "
                    "Currently hccl data does not support exporting files other than timeline."
        })

    @staticmethod
    def _get_msproftx_data(configs: dict, params: dict) -> any:
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.SUMMARY:
            return MsprofTxViewer(configs, params).get_summary_data()
        return MsprofTxViewer(configs, params).get_timeline_data()

    @staticmethod
    def _get_stars_soc_data(configs: dict, params: dict) -> any:
        return StarsSocView(configs, params).get_timeline_data()

    @staticmethod
    def _get_stars_chip_trans_data(configs: dict, params: dict) -> any:
        return StarsChipTransView(configs, params).get_timeline_data()

    @staticmethod
    def _get_thread_group_data(configs: dict, params: dict) -> any:
        return ThreadGroupViewer(configs, params).get_timeline_data()

    @staticmethod
    def _get_low_power_data(configs: dict, params: dict) -> any:
        return LowPowerViewer(configs, params).get_timeline_data()

    @staticmethod
    def _get_biu_perf_timeline(configs: dict, params: dict) -> any:
        _ = configs
        return BiuPerfViewer(params.get(StrConstant.PARAM_RESULT_DIR)).get_timeline()

    @staticmethod
    def _get_acc_pmu(configs: dict, params: dict) -> any:
        return AccPmuViewer(configs, params).get_timeline_data()

    @classmethod
    def export_data(cls: any, params: dict) -> str:
        """
        export data, support different data types and export types
        :param params: param object
        :return: result of export
        """
        if params.get(StrConstant.DATA_TYPE) is None:
            return json.dumps({"status": NumberConstant.ERROR, "info": "Parameter data_type is none."})
        if not cls.init_cfg_finished:
            cls.LOCK.acquire()
            if not cls.init_cfg_finished:
                cls._load_export_data_config()
            cls.LOCK.release()
        configs = cls._get_configs_with_data_type(params.get(StrConstant.PARAM_DATA_TYPE))
        if configs.get(StrConstant.CONFIG_HANDLER) is not None \
                and hasattr(cls, configs.get(StrConstant.CONFIG_HANDLER)):
            handler = getattr(cls, configs.get(StrConstant.CONFIG_HANDLER))
            if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.SUMMARY:
                headers, data, _ = handler(configs, params)
                return MsprofDataStorage().export_summary_data(headers, Utils.data_processing_with_decimals(data),
                                                               params)
            data = handler(configs, params)
            if isinstance(data, EmptyClass):
                return json.dumps({"status": NumberConstant.ERROR, "info": "Unable to get prof data."})
            cls.add_timeline_data(params, data)
            return MsprofDataStorage().export_timeline_data_to_json(data, params)
        return json.dumps({
            "status": NumberConstant.ERROR,
            "info": "Unable to handler data type %s." % params.get(StrConstant.PARAM_DATA_TYPE)
        })

    @classmethod
    def add_timeline_data(cls: any, params: dict, data: any) -> None:
        """
        add timeline data to bulk
        :param params:
        :param data:
        :return:
        """
        filter_list = ["msprof", "ai_stack_time", "step_trace", "thread_group"]
        if params.get(StrConstant.PARAM_DATA_TYPE) not in filter_list:
            MsprofTimeline().add_export_data(data, params.get(StrConstant.PARAM_DATA_TYPE))

    @classmethod
    def _load_export_data_config(cls: any) -> None:
        """
        load export configuration
        :return: None
        """
        cls.cfg_parser = ConfigManager.get(ConfigManager.MSPROF_EXPORT_DATA)
        cls.init_cfg_finished = True

    @classmethod
    def _get_configs_with_data_type(cls: any, data_type: str) -> dict:
        """
        get configs according to data type
        :param data_type: data type
        :return: config parser
        """
        configs = {}
        if cls.cfg_parser:
            if cls.cfg_parser.has_option(data_type, StrConstant.CONFIG_HANDLER):
                configs[StrConstant.CONFIG_HANDLER] = cls.cfg_parser.get(data_type,
                                                                         StrConstant.CONFIG_HANDLER)

            configs[StrConstant.CONFIG_HEADERS] = []
            if cls.cfg_parser.has_option(data_type, StrConstant.CONFIG_HEADERS):
                headers = cls.cfg_parser.get(data_type, StrConstant.CONFIG_HEADERS)
                if headers:
                    configs[StrConstant.CONFIG_HEADERS] = headers.split(",")

            cls._get_configs_with_option(configs, data_type, StrConstant.CONFIG_DB)

            cls._get_configs_with_option(configs, data_type, StrConstant.CONFIG_COLUMNS)

            cls._get_configs_with_option(configs, data_type, StrConstant.CONFIG_TABLE)

            configs[StrConstant.CONFIG_UNUSED_COLS] = []
            if cls.cfg_parser.has_option(data_type, StrConstant.CONFIG_UNUSED_COLS):
                unused_cols = cls.cfg_parser.get(data_type, StrConstant.CONFIG_UNUSED_COLS)
                if unused_cols:
                    configs[StrConstant.CONFIG_UNUSED_COLS] = unused_cols.split(",")
        return configs

    @classmethod
    def _get_configs_with_option(cls: any, configs: dict, data_type: str, option: str) -> None:
        if cls.cfg_parser.has_option(data_type, option):
            configs[option] = cls.cfg_parser.get(data_type, option)

    @classmethod
    def _get_ai_core_sample_based_data(cls: any, configs: dict, params: dict) -> any:
        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.TIMELINE:
            aicore_utilization_param = {
                'project_path': params.get(StrConstant.PARAM_RESULT_DIR),
                'device_id': params.get(StrConstant.PARAM_DEVICE_ID),
                'start_time': NumberConstant.DEFAULT_START_TIME,
                'end_time': NumberConstant.DEFAULT_END_TIME
            }
            result = get_aicore_utilization_timeline(aicore_utilization_param)
            return result
        params[StrConstant.CORE_DATA_TYPE] = StrConstant.AI_CORE_PMU_EVENTS
        return get_core_sample_data(params.get(StrConstant.PARAM_RESULT_DIR),
                                    configs.get(StrConstant.CONFIG_DB),
                                    params.get(StrConstant.PARAM_DEVICE_ID), params)

    @classmethod
    def _get_aiv_sample_based_data(cls: any, configs: dict, params: dict) -> any:
        params[StrConstant.CORE_DATA_TYPE] = StrConstant.AI_VECTOR_CORE_PMU_EVENTS
        return get_core_sample_data(params.get(StrConstant.PARAM_RESULT_DIR),
                                    configs.get(StrConstant.CONFIG_DB),
                                    params.get(StrConstant.PARAM_DEVICE_ID), params)
