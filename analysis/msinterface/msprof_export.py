#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import json
import logging
import os

from analyzer.data_analysis_factory import DataAnalysisFactory
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.ai_stack_data_check_manager import AiStackDataCheckManager
from common_func.common import error
from common_func.common import print_info
from common_func.common import warn
from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.data_check_manager import DataCheckManager
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.host_data_check_manager import HostDataCheckManager
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.msprof_common import analyze_collect_data
from common_func.msprof_common import check_collection_dir
from common_func.msprof_common import check_path_valid
from common_func.msprof_common import get_path_dir
from common_func.msprof_common import prepare_for_parse
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from common_func.system_data_check_manager import SystemDataCheckManager
from common_func.utils import Utils
from framework.file_dispatch import FileDispatch
from framework.load_info_manager import LoadInfoManager
from msinterface.msprof_export_data import MsProfExportDataUtils
from msinterface.msprof_job_summary import MsprofJobSummary
from msinterface.msprof_timeline import MsprofTimeline
from profiling_bean.db_dto.step_trace_dto import IterationRange
from msmodel.step_trace.ts_track_model import TsTrackModel
from profiling_bean.db_dto.hwts_rec_dto import HwtsRecDto
from profiling_bean.db_dto.step_trace_dto import StepTraceDto
from profiling_bean.prof_enum.export_data_type import ExportDataType
from tuning.cluster.cluster_tuning_facade import ClusterTuningFacade
from tuning.cluster_tuning import ClusterTuning
from tuning.profiling_tuning import ProfilingTuning
from viewer.tuning_view import TuningView


class ExportCommand:
    """
    The class for handle export command.
    """
    FILE_NAME = os.path.basename(__file__)
    EXPORT_HANDLE_MAP = {
        MsProfCommonConstant.TIMELINE: [
            {'export_type': ExportDataType.STEP_TRACE,
             'handler': AiStackDataCheckManager.contain_training_trace_data_or_step},
            {'export_type': ExportDataType.ACL,
             'handler': AiStackDataCheckManager.contain_acl_data},
            {'export_type': ExportDataType.GE,
             'handler': AiStackDataCheckManager.contain_ge_model_time_data},
            {'export_type': ExportDataType.GE_OP_EXECUTE,
             'handler': AiStackDataCheckManager.contain_ge_op_execute_data},
            {'export_type': ExportDataType.RUNTIME_API,
             'handler': AiStackDataCheckManager.contain_runtime_api_data},
            {'export_type': ExportDataType.TASK_TIME,
             'handler': AiStackDataCheckManager.contain_core_cpu_reduce_data},
            {'export_type': ExportDataType.HBM, 'handler': SystemDataCheckManager.contain_hbm_data},
            {'export_type': ExportDataType.DDR, 'handler': SystemDataCheckManager.contain_ddr_data},
            {'export_type': ExportDataType.PCIE,
             'handler': SystemDataCheckManager.contain_pcie_data},
            {'export_type': ExportDataType.HCCS,
             'handler': SystemDataCheckManager.contain_hccs_data},
            {'export_type': ExportDataType.NIC, 'handler': SystemDataCheckManager.contain_nic_data},
            {'export_type': ExportDataType.ROCE,
             'handler': SystemDataCheckManager.contain_roce_data},
            {'export_type': ExportDataType.LLC_READ_WRITE,
             'handler': SystemDataCheckManager.contain_read_write_data},
            {'export_type': ExportDataType.LLC_AICPU,
             'handler': SystemDataCheckManager.contain_llc_capacity_data},
            {'export_type': ExportDataType.LLC_CTRLCPU,
             'handler': SystemDataCheckManager.contain_llc_capacity_data},
            {'export_type': ExportDataType.LLC_BANDWIDTH,
             'handler': SystemDataCheckManager.contain_llc_bandwidth_data},
            {'export_type': ExportDataType.AI_STACK_TIME,
             'handler': AiStackDataCheckManager.contain_ai_stack_time_data},
            {'export_type': ExportDataType.AI_CORE_UTILIZATION,
             'handler': AiStackDataCheckManager.contain_ai_core_sample_based},
            {'export_type': ExportDataType.HOST_CPU_USAGE,
             'handler': HostDataCheckManager.contain_host_cpuusage_data},
            {'export_type': ExportDataType.HOST_MEM_USAGE,
             'handler': HostDataCheckManager.contain_host_mem_usage_data},
            {'export_type': ExportDataType.HOST_NETWORK_USAGE,
             'handler': HostDataCheckManager.contain_host_network_usage_data},
            {'export_type': ExportDataType.HOST_DISK_USAGE,
             'handler': HostDataCheckManager.contain_host_disk_usage_data},
            {'export_type': ExportDataType.OS_RUNTIME_API,
             'handler': HostDataCheckManager.contain_runtime_api_data},
            {'export_type': ExportDataType.FFTS_SUB_TASK_TIME,
             'handler': AiStackDataCheckManager.contain_stars_soc_data},
            {'export_type': ExportDataType.HCCL,
             'handler': AiStackDataCheckManager.contain_hccl_hcom_data},
            {'export_type': ExportDataType.MSPROF_TX,
             'handler': AiStackDataCheckManager.contain_msproftx_data},
            {'export_type': ExportDataType.STARS_SOC,
             'handler': AiStackDataCheckManager.contain_stars_soc_profiler_data},
            {'export_type': ExportDataType.STARS_CHIP_TRANS,
             'handler': AiStackDataCheckManager.contain_stars_chip_trans_data},
            {'export_type': ExportDataType.THREAD_GROUP,
             'handler': AiStackDataCheckManager.contain_thread_group_data},
            {'export_type': ExportDataType.LOW_POWER,
             'handler': AiStackDataCheckManager.contain_stars_low_power_data},
            {'export_type': ExportDataType.BIU_PERF,
             'handler': AiStackDataCheckManager.contain_biu_perf_data},
            {'export_type': ExportDataType.ACC_PMU,
             'handler': AiStackDataCheckManager.contain_acc_pmu_data},
            {'export_type': ExportDataType.MSPROF,
             'handler': lambda result_dir, device_id=None: True}
        ],
        MsProfCommonConstant.SUMMARY: [
            {'export_type': ExportDataType.TASK_TIME,
             'handler': AiStackDataCheckManager.contain_task_time_task},
            {'export_type': ExportDataType.L2_CACHE,
             'handler': AiStackDataCheckManager.contain_l2_cache_data},
            {'export_type': ExportDataType.STEP_TRACE,
             'handler': AiStackDataCheckManager.contain_training_trace_data_or_step},
            {'export_type': ExportDataType.GE_OP_EXECUTE,
             'handler': AiStackDataCheckManager.contain_ge_op_execute_data},
            {'export_type': ExportDataType.OP_SUMMARY,
             'handler': AiStackDataCheckManager.contain_op_summary_data},
            {'export_type': ExportDataType.OP_STATISTIC,
             'handler': AiStackDataCheckManager.contain_op_static_data},
            {'export_type': ExportDataType.AICPU,
             'handler': AiStackDataCheckManager.contain_dp_aicpu_data},
            {'export_type': ExportDataType.DP,
             'handler': AiStackDataCheckManager.contain_data_preprocess_dp_data},
            {'export_type': ExportDataType.CPU_USAGE,
             'handler': SystemDataCheckManager.contain_cpu_usage_data},
            {'export_type': ExportDataType.PROCESS_CPU_USAGE,
             'handler': SystemDataCheckManager.contain_pid_cpu_usage_data},
            {'export_type': ExportDataType.SYS_MEM,
             'handler': SystemDataCheckManager.contains_sys_memory_data},
            {'export_type': ExportDataType.PROCESS_MEM,
             'handler': SystemDataCheckManager.contains_pid_memory_data},
            {'export_type': ExportDataType.HBM, 'handler': SystemDataCheckManager.contain_hbm_data},
            {'export_type': ExportDataType.DDR, 'handler': SystemDataCheckManager.contain_ddr_data},
            {'export_type': ExportDataType.PCIE,
             'handler': SystemDataCheckManager.contain_pcie_data},
            {'export_type': ExportDataType.HCCS,
             'handler': SystemDataCheckManager.contain_hccs_data},
            {'export_type': ExportDataType.DVPP,
             'handler': SystemDataCheckManager.contain_dvpp_data},
            {'export_type': ExportDataType.NIC, 'handler': SystemDataCheckManager.contain_nic_data},
            {'export_type': ExportDataType.ROCE,
             'handler': SystemDataCheckManager.contain_roce_data},
            {'export_type': ExportDataType.LLC_READ_WRITE,
             'handler': SystemDataCheckManager.contain_read_write_data},
            {'export_type': ExportDataType.LLC_AICPU,
             'handler': SystemDataCheckManager.contain_llc_capacity_data},
            {'export_type': ExportDataType.LLC_CTRLCPU,
             'handler': SystemDataCheckManager.contain_llc_capacity_data},
            {'export_type': ExportDataType.LLC_BANDWIDTH,
             'handler': SystemDataCheckManager.contain_llc_bandwidth_data},
            {'export_type': ExportDataType.AI_CPU_TOP_FUNCTION,
             'handler': SystemDataCheckManager.contain_ai_cpu_data},
            {'export_type': ExportDataType.AI_CPU_PMU_EVENTS,
             'handler': SystemDataCheckManager.contain_ai_cpu_data},
            {'export_type': ExportDataType.CTRL_CPU_TOP_FUNCTION,
             'handler': SystemDataCheckManager.contain_ctrl_cpu_data},
            {'export_type': ExportDataType.CTRL_CPU_PMU_EVENTS,
             'handler': SystemDataCheckManager.contain_ctrl_cpu_data},
            {'export_type': ExportDataType.TS_CPU_TOP_FUNCTION,
             'handler': SystemDataCheckManager.contain_ts_cpu_data},
            {'export_type': ExportDataType.TS_CPU_PMU_EVENTS,
             'handler': SystemDataCheckManager.contain_ts_cpu_data},
            {'export_type': ExportDataType.ACL,
             'handler': AiStackDataCheckManager.contain_acl_data},
            {'export_type': ExportDataType.ACL_STATISTIC,
             'handler': AiStackDataCheckManager.contain_acl_data},
            {'export_type': ExportDataType.FUSION_OP,
             'handler': AiStackDataCheckManager.contain_fusion_op_data},
            {'export_type': ExportDataType.RUNTIME_API,
             'handler': AiStackDataCheckManager.contain_runtime_api_data},
            {'export_type': ExportDataType.AI_STACK_TIME,
             'handler': AiStackDataCheckManager.contain_ai_stack_time_data},
            {'export_type': ExportDataType.AI_CORE_UTILIZATION,
             'handler': AiStackDataCheckManager.contain_ai_core_sample_based},
            {'export_type': ExportDataType.AI_VECTOR_CORE_UTILIZATION,
             'handler': AiStackDataCheckManager.contain_aiv_core_sample_based},
            {'export_type': ExportDataType.OS_RUNTIME_STATISTIC,
             'handler': HostDataCheckManager.contain_runtime_api_data},
            {'export_type': ExportDataType.MSPROF_TX,
             'handler': AiStackDataCheckManager.contain_msproftx_data}
        ]
    }
    MODEL_ID = "model_id"
    INPUT_MODEL_ID = "input_model_id"

    def __init__(self: any, command_type: str, args: any) -> None:
        self.command_type = command_type
        self.collection_path = os.path.realpath(args.collection_path)
        self.iteration_id = getattr(args, "iteration_id", NumberConstant.DEFAULT_ITER_ID)
        self.iteration_count = getattr(args, "iteration_count", NumberConstant.DEFAULT_ITER_COUNT)
        self.sample_config = None
        self.export_format = getattr(args, "export_format", None)
        self.list_map = {
            'export_type_list': [],
            'devices_list': '',
            'model_id': getattr(args, self.MODEL_ID),
            'input_model_id': args.model_id is not None
        }
        self.iteration_range = None
        self._cluster_params = {'is_cluster_scene': False, 'cluster_path': []}

    @staticmethod
    def get_model_id_set(result_dir: str, db_name: str, table_name: str) -> any:
        """
        get model id set
        :param result_dir: result dir
        :param db_name: db name
        :param table_name: table name
        :return: model_ids_set
        """
        db_path = PathManager.get_db_path(result_dir, db_name)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not conn or not curs or not DBManager.judge_table_exist(curs, table_name):
            logging.warning(
                "Can not get model id from framework data, maybe framework data is not collected,"
                " try to export data with no framework data.")
            DBManager.destroy_db_connect(conn, curs)
            return set()

        sql = "select model_id from {0}".format(table_name)
        model_ids = DBManager.fetch_all_data(curs, sql)
        model_ids_set = {model_id[0] for model_id in model_ids}
        DBManager.destroy_db_connect(conn, curs)

        return model_ids_set

    @staticmethod
    def _update_model_and_index(result_dir: str, trace_data_dict: dict) -> list:
        conn, curs = DBManager.check_connect_db(result_dir, DBNameConstant.DB_HWTS_REC)
        if not (conn and curs):
            return []
        sql = "select iter_id from {} where ai_core_num > 0".format(DBNameConstant.TABLE_HWTS_ITER_SYS)
        iter_data = DBManager.fetch_all_data(curs, sql, dto_class=HwtsRecDto)
        DBManager.destroy_db_connect(conn, curs)
        result = dict()
        for data in iter_data:
            if data.iter_id in trace_data_dict.keys():
                model_id_times = result.get(trace_data_dict.get(data.iter_id, 0), 0)
                result[trace_data_dict.get(data.iter_id, 0)] = model_id_times + 1
        return list(zip(result.keys(), result.values()))

    def process(self: any) -> None:
        """
        handle export command
        :return: None
        """
        check_path_valid(self.collection_path, False)
        if DataCheckManager.contain_info_json_data(self.collection_path):  # find profiling data dir
            InfoConfReader().load_info(self.collection_path)
            self._handle_export(os.path.realpath(self.collection_path))
            self._show_tuning_result(os.path.realpath(self.collection_path))
        else:
            self._process_sub_dirs()
            if self._cluster_params.get('is_cluster_scene', False):
                self._show_cluster_tuning()

    def _set_default_model_id(self, result_dir, model_match_set, ge_data_set):
        conn, curs = DBManager.check_connect_db(result_dir, DBNameConstant.DB_STEP_TRACE)
        if not (conn and curs):
            return min(model_match_set)
        sql = "select model_id, max(index_id) from {} group by model_id".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        model_and_index = list(filter(lambda x: x[0] in model_match_set, DBManager.fetch_all_data(curs, sql)))

        if not ge_data_set:
            trace_data = DBManager.fetch_all_data(
                curs, "select model_id, iter_id from {}".format(DBNameConstant.TABLE_STEP_TRACE_DATA),
                dto_class=StepTraceDto)
            model_and_index = self._update_model_and_index(result_dir,
                                                           {data.iter_id: data.model_id for data in trace_data})
        model_and_index.sort(key=lambda x: x[1])
        DBManager.destroy_db_connect(conn, curs)
        return model_and_index.pop()[0] if model_and_index else min(model_match_set)

    def _add_export_type(self: any, result_dir: str) -> None:
        export_map = self.EXPORT_HANDLE_MAP.get(self.command_type, [])
        for item in export_map:
            if item['handler'](result_dir, self.sample_config.get('devices', 0)):
                self.list_map.get('export_type_list').append(item)

    def _is_iteration_range_valid(self, project_path):
        with TsTrackModel(project_path, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE_DATA]) as _trace:
            step_trace = _trace.get_max_index_id_with_model(self.list_map.get(self.MODEL_ID))

        if self.iteration_count < NumberConstant.DEFAULT_ITER_ID \
                or self.iteration_count > IterationRange.MAX_ITERATION_COUNT:
            error(self.FILE_NAME, f'The iteration count {self.iteration_count} is invalid '
                                  f'for model id {self.list_map.get(self.MODEL_ID)}. '
                                  f'Must be greater than 0 and less than or equal to '
                                  f'{IterationRange.MAX_ITERATION_COUNT}. Please enter a valid iteration count.')
            return False

        if step_trace and self.iteration_id + self.iteration_count - 1 > step_trace.index_id:
            error(self.FILE_NAME,
                  f'The current iteration id is {self.iteration_id}, '
                  f'and you want to export {self.iteration_count} rounds of iterations, '
                  f'but the current maximum iteration is {step_trace.index_id} for model id '
                  f'{self.list_map.get(self.MODEL_ID)}. '
                  f'Please enter a valid iteration id and iteration count.')
            return False
        return True

    def _check_index_id(self: any, project_path: str) -> None:
        """
        check index id
        :param project_path: path to get profiling scene
        :return: void
        """
        ProfilingScene().init(project_path)
        if not ProfilingScene().is_step_trace() and self.iteration_count > NumberConstant.DEFAULT_ITER_COUNT:
            warn(self.FILE_NAME, f'Param of "iteration-count" is {self.iteration_count}, '
                                 f'but it is unnecessary without step trace data.')
            self.iteration_count = NumberConstant.DEFAULT_ITER_COUNT
            return

        if self.iteration_id < NumberConstant.DEFAULT_ITER_ID:
            error(self.FILE_NAME,
                  f'The iteration id {self.iteration_id} is invalid for model id '
                  f'{self.list_map.get(self.MODEL_ID)}. Must be greater than 0. Please enter a valid iteration id.')
            raise ProfException(ProfException.PROF_INVALID_STEP_TRACE_ERROR)

        if not self._is_iteration_range_valid(project_path):
            raise ProfException(ProfException.PROF_INVALID_STEP_TRACE_ERROR)

    def _analyse_sample_config(self: any, result_dir: str) -> None:
        self.sample_config = ConfigMgr.read_sample_config(result_dir)

    def _analyse_data(self: any, result_dir: str) -> None:
        is_data_analyzed = FileManager.is_analyzed_data(result_dir)
        if not is_data_analyzed:
            analyze_collect_data(result_dir, self.sample_config)
        else:
            print_info(self.FILE_NAME, 'The data in "%s" has been analyzed.'
                       % result_dir)

    def _check_model_id(self: any, result_dir: str) -> None:
        """
        check model id
        :param result_dir: initialize profiling_scene
        :return: void
        """
        profiling_scene = ProfilingScene()
        profiling_scene.init(result_dir)

        if not profiling_scene.is_step_trace():
            self.list_map[self.MODEL_ID] = Constant.GE_OP_MODEL_ID
            return

        model_ids_ge = ExportCommand.get_model_id_set(
            result_dir, DBNameConstant.DB_GE_INFO, DBNameConstant.TABLE_GE_TASK)

        model_ids_step = ExportCommand.get_model_id_set(
            result_dir, DBNameConstant.DB_STEP_TRACE, DBNameConstant.TABLE_STEP_TRACE_DATA)

        model_match_set = model_ids_step
        if model_ids_ge and model_ids_step:
            not_match_set = model_ids_ge - model_ids_step
            if not_match_set:
                logging.warning("step trace data miss model id.")
                logging.debug("step trace data miss %s.", not_match_set)
            model_match_set = model_ids_ge & model_ids_step
        else:
            logging.warning("ge step info data miss model id.")

        if not self.list_map.get(self.INPUT_MODEL_ID):
            self.list_map[self.MODEL_ID] = self._set_default_model_id(result_dir, model_match_set, model_ids_ge)
            if Utils.is_single_op_graph_mix(result_dir):
                self.list_map[self.MODEL_ID] = Constant.GE_OP_MODEL_ID
            return

        if self.list_map.get(self.MODEL_ID) not in model_match_set:
            message = f"The model id {self.list_map.get(self.MODEL_ID)} is invalid. "\
                      f"Must select from {model_match_set}. Please enter a valid model id."
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR, message)

    def _prepare_for_export(self: any, result_dir: str) -> None:
        LoadInfoManager.load_info(result_dir)
        self.list_map['export_type_list'] = []
        self._analyse_sample_config(result_dir)
        self._analyse_data(result_dir)
        self._add_export_type(result_dir)
        self._check_model_id(result_dir)
        self._check_index_id(result_dir)

        self.iteration_range = IterationRange(model_id=self.list_map.get('model_id'),
                                              iteration_id=self.iteration_id,
                                              iteration_count=self.iteration_count)
        MsprofTimeline().set_iteration_info(result_dir, self.iteration_range)
        device_lst = InfoConfReader().get_device_list()
        if device_lst:
            self.list_map.update({'devices_list': device_lst})
            sample_json = {
                StrConstant.SAMPLE_CONFIG_PROJECT_PATH: result_dir,
                StrConstant.PARAM_DEVICE_ID: self.list_map.get('devices_list')[0],
                StrConstant.PARAM_ITER_ID: self.iteration_range,
                StrConstant.PARAM_JOB_ID: MsProfCommonConstant.DEFAULT_JOB,
            }
            file_dispatch = FileDispatch(sample_json)
            file_dispatch.dispatch_calculator()
            analysis_factory = DataAnalysisFactory(sample_json)
            analysis_factory.run()

        if self.command_type == MsProfCommonConstant.SUMMARY:
            check_path_valid(PathManager.get_summary_dir(result_dir), True)
        else:
            check_path_valid(PathManager.get_timeline_dir(result_dir), True)

        if len(self.list_map.get('export_type_list')) == 0:
            print_info(self.FILE_NAME, 'There is no %s data to export for "%s". Please '
                                       'check the path.' % (self.command_type, result_dir))

    def _handle_export_data(self: any, params: dict) -> None:
        result = json.loads(MsProfExportDataUtils.export_data(params))
        if result.get('status', NumberConstant.EXCEPTION) == NumberConstant.SUCCESS:
            self._print_export_info(params, result.get('data', []))
        else:
            if result.get('status', NumberConstant.EXCEPTION) == NumberConstant.ERROR:
                error(self.FILE_NAME, result.get('info', ""))
            else:
                warn(self.FILE_NAME, result.get('info', ""))

    def _print_export_info(self: any, params: dict, data: str) -> None:
        export_info = 'The {0} {1} data has been ' \
                      'exported to "{2}".'.format(params.get(StrConstant.PARAM_DATA_TYPE),
                                                  self.command_type,
                                                  data)

        if params.get(StrConstant.PARAM_DEVICE_ID) is not None:
            export_info = 'The {0} {1} data of device {2} for iteration {3} has been ' \
                          'exported to "{4}".'.format(params.get(StrConstant.PARAM_DATA_TYPE),
                                                      self.command_type,
                                                      params.get(StrConstant.PARAM_DEVICE_ID),
                                                      params.get(StrConstant.PARAM_ITER_ID),
                                                      data)
        print_info(self.FILE_NAME, export_info)

    def _handle_export(self: any, result_dir: str) -> None:
        self._prepare_export(result_dir)
        try:
            for event in self.list_map.get('export_type_list', []):
                if not self.list_map.get('devices_list', []):
                    self._export_data(event, None, result_dir)
                    continue
                for device_id in self.list_map.get('devices_list', []):
                    self._export_data(event, device_id, result_dir)
        except ProfException:
            warn(MsProfCommonConstant.COMMON_FILE_NAME,
                 'Analysis data in "%s" failed. Maybe the data is incomplete.' % result_dir)

    def _prepare_export(self: any, result_dir: str) -> None:
        check_collection_dir(result_dir)
        prepare_for_parse(result_dir)
        self._prepare_for_export(result_dir)

    def _export_data(self: any, event: dict, device_id: str, result_dir: str) -> None:
        export_data_type = event.get('export_type', ExportDataType.INVALID).name.lower()
        if not event['handler'](result_dir, device_id):
            warn(self.FILE_NAME, 'There is no %s data in device %s.'
                 % (export_data_type, device_id))
            return
        print_info(self.FILE_NAME,
                   'Start to export %s %s data ...' % (export_data_type, self.command_type))
        params = {
            StrConstant.PARAM_DATA_TYPE: export_data_type,
            StrConstant.PARAM_RESULT_DIR: result_dir,
            StrConstant.PARAM_DEVICE_ID: device_id,
            StrConstant.PARAM_JOB_ID: MsProfCommonConstant.DEFAULT_JOB,
            StrConstant.PARAM_EXPORT_TYPE: self.command_type,
            StrConstant.PARAM_ITER_ID: self.iteration_range,
            StrConstant.PARAM_EXPORT_FORMAT: self.export_format,
            StrConstant.PARAM_MODEL_ID: self.list_map.get("model_id")
        }

        self._handle_export_data(params)

    def _show_tuning_result(self: any, result_dir: str) -> None:
        if self.command_type != MsProfCommonConstant.SUMMARY or not self.list_map.get("devices_list", []) \
                or not all(Utils.generator_to_list(device_id.isdigit()
                                                   for device_id in self.list_map.get("devices_list", []))):
            return
        ProfilingTuning.run(result_dir)
        sample_config = {}
        tuning_view = TuningView(result_dir, sample_config, self.list_map.get("devices_list")[0])
        tuning_view.show_by_dev_id()

    def _show_cluster_tuning(self) -> None:
        if self.command_type != MsProfCommonConstant.SUMMARY:
            return
        ClusterTuning(self._cluster_params.get('cluster_path')).run()
        params = {"collection_path": self.collection_path,
                  "model_id": 0,
                  "npu_id": -1,
                  "iteration_id": self.iteration_range.iteration_id}
        try:
            ClusterTuningFacade(params).process()
        except ProfException:
            warn(self.FILE_NAME, 'Cluster Tuning did not complete!')

    def _update_cluster_params(self: any, sub_path: str, is_cluster: bool) -> None:
        if is_cluster:
            self._cluster_params['is_cluster_scene'] = True
            self._cluster_params.setdefault('cluster_path', []).append(sub_path)

    def _process_sub_dirs(self: any, sub_path: str = '', is_cluster: bool = False) -> None:
        collect_path = self.collection_path
        if sub_path:
            collect_path = os.path.join(self.collection_path, sub_path)
        sub_dirs = get_path_dir(collect_path)
        for sub_dir in sub_dirs:  # result_dir
            if sub_dir != StrConstant.TIMELINE_PATH:
                sub_path = os.path.realpath(
                    os.path.join(collect_path, sub_dir))
                check_path_valid(sub_path, False)
                if DataCheckManager.contain_info_json_data(sub_path):
                    self._update_cluster_params(sub_path, is_cluster)
                    InfoConfReader().load_info(sub_path)
                    self._handle_export(sub_path)
                    self._show_tuning_result(sub_path)
                elif sub_path and is_cluster:
                    warn(self.FILE_NAME, 'Invalid parsing dir("%s"), -dir must be profiling data dir '
                                         'such as PROF_XXX_XXX_XXX' % collect_path)
                else:
                    self._process_sub_dirs(sub_dir, is_cluster=True)
                self.list_map['devices_list'] = ''
        job_summary = MsprofJobSummary(collect_path)
        job_summary.export()
