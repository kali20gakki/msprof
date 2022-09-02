import sqlite3
import unittest
from unittest import mock
from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from msparser.aicpu.parse_aicpu_data import ParseAiCpuData
from profiling_bean.prof_enum.data_tag import DataTag
from sqlite.db_manager import DBManager

NAMESPACE = 'msparser.aicpu.parse_aicpu_data'


class TestParseAiCpuData(unittest.TestCase):
    file_list = {DataTag.AI_CPU: ['.data.0.slice_0']}

    def test_read_and_analysis_ai_cpu(self):
        data = b'[179544599] [sessionID:0] Create session start:179544678, Create session ' \
               b'end:179545034\n\x00Thread[ 4] process device[0]:kernelType[ 1] task end, ' \
               b'submitTick=17954444629,scheduleTick=17954446093, tickBeforeRun=17954460606,' \
               b'tickAfterRun=17954506180, dispatchTime=159 us, totalTime=615 ' \
               b'us.\n\x00[180171254] Node:fpn/l2/kernel/Initializer/random_uniform/RandomUniform, ' \
               b'Run start:180171763 18017176361, compute start:180171794, memcpy start:180172470, ' \
               b'memcpy end:180172473, Run end:180172501 18017250105\x00Thread[ 8] process ' \
               b'device[0]:kernelType[ 1] task end, submitTick=18017112265,scheduleTick=18017113183, ' \
               b'tickBeforeRun=18017125941,tickAfterRun=18017260896, dispatchTime=136 us, ' \
               b'totalTime=1486 us.\x00[180173067] ' \
               b'Node:fpn/l3/kernel/Initializer/random_uniform/RandomUniform, ' \
               b'Run start:180173448 18017344817, compute start:180173464, memcpy start:180174717, ' \
               b'memcpy end:180174720, Run end:180174743 18017474309\x00Thread[ 3] ' \
               b'process device[0]:kernelType[ 1] task end, submitTick=18017296272,' \
               b'scheduleTick=18017297109, tickBeforeRun=18017307033,tickAfterRun=18017484178, ' \
               b'dispatchTime=107 us, totalTime=1879 us.\x00[180175138] '
        with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
             mock.patch(NAMESPACE + '.ParseAiCpuData._ParseAiCpuData__analysis_ai_cpu'), \
             mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 10}):
            check = ParseAiCpuData(self.file_list, CONFIG)
            InfoConfReader()._info_json = {'devices': 0}
            result = check._ParseAiCpuData__read_and_analysis_ai_cpu('test')
        self.assertEqual(result, None)
        with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
             mock.patch(NAMESPACE + '.ParseAiCpuData._ParseAiCpuData__analysis_ai_cpu'), \
             mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 10}):
            check = ParseAiCpuData(self.file_list, CONFIG)
            InfoConfReader()._info_json = {'devices': 1}
            result = check._ParseAiCpuData__read_and_analysis_ai_cpu('test')
        self.assertEqual(result, None)

    def test_create_db(self):
        with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
             mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test\\db'), \
             mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=(True, True)), \
             mock.patch('common_func.iter_recorder.MsprofIteration.get_iteration_end_dict', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.BatchCounter.init'), \
             mock.patch(NAMESPACE + '.IterRecorder'), \
             mock.patch(NAMESPACE + '.DBManager.sql_create_general_table', return_value=""), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'):
            check = ParseAiCpuData(self.file_list, CONFIG)
            result = check._ParseAiCpuData__create_db()

    def test_insert_data(self):
        manager = DBManager()
        res = manager.create_table('test.db')
        with mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 10}):
            check = ParseAiCpuData(self.file_list, CONFIG)
            result = check._ParseAiCpuData__insert_data()
            self.assertEqual(result, None)
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test\\data'), \
             mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=res), \
             mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
             mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 10}):
            check = ParseAiCpuData(self.file_list, CONFIG)
            check.ai_cpu_datas = [[65535, 65535, 50502590923, 50502595804, 'GreaterEqual',
                                   0.027, 0.001, 0.04881, 0.062, 0.51]]
            check._ParseAiCpuData__insert_data()
        manager.destroy(res)
        db_manager = DBManager()
        res = db_manager.create_table('ai_cpu.db')
        res[1].execute("CREATE TABLE IF NOT EXISTS ai_cpu_datas(stream_id INTEGER,task_id INTEGER,"
                       "sys_start INTEGER,sys_end INTEGER,node_name TEXT,compute_time REAL,"
                       "memcpy_time REAL,task_time REAL,dispatch_time REAL,total_time REAL)")
        with mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='test\\data'), \
             mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                        return_value=res), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
             mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 10}):
            check = ParseAiCpuData(self.file_list, CONFIG)
            check.ai_cpu_datas = [[65535, 65535, 50502590923, 50502595804, 'GreaterEqual',
                                   0.027, 0.001, 0.04881, 0.062, 0.51]]
            check._ParseAiCpuData__insert_data()
        res[1].execute('drop table ai_cpu_datas')
        db_manager.destroy(res)

    def test_analysis_ai_cpu(self):
        ai_cpu_datas = (
            '[180171254] Node:fpn/l2/kernel/Initializer/random_uniform/RandomUniform, '
            'Run start:180171763 18017176361, compute start:180171794, memcpy start:180172470, '
            'memcpy end:180172473, Run end:180172501 18017250105',
            'Thread[ 8] processdevice[0]:kernelType[ 1] task end, submitTick=18017112265,'
            'scheduleTick=18017113183, tickBeforeRun=18017125941,tickAfterRun=18017260896, '
            'dispatchTime=136 us, totalTime=1486 us.'
        )
        wrong_ai_cpu_datas = (
            '[180171254] Node:fpn/l2/kernel/Initializer/random_uniform/RandomUniform, '
            'Run start:180171763 18017176361, compute start:180171794, memcpy start:180172470, '
            'memcpy end:180172473, Run end:180172501 18017170105',
            'Thread[ 8] processdevice[0]:kernelType[ 1] task end, submitTick=18017112265,'
            'scheduleTick=18017113183, tickBeforeRun=18017125941,tickAfterRun=18017260896, '
            'dispatchTime=136 us, totalTime=1486 us.'
        )

        with mock.patch('common_func.msprof_iteration.Utils.is_step_scene', return_value=True), \
             mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict',
                        return_value={1: 28017250105}):
            check = ParseAiCpuData(self.file_list, CONFIG)

        InfoConfReader()._info_json = {"DeviceInfo": [
            {"id": 0, "env_type": 3, "ctrl_cpu_id": "ARMv8_Cortex_A55", "ctrl_cpu_core_num": 4,
             "ctrl_cpu_endian_little": 1, "ts_cpu_core_num": 1, "ai_cpu_core_num": 4, "ai_core_num": 2,
             "ai_cpu_core_id": 4, "ai_core_id": 0, "aicpu_occupy_bitmap": 240, "ctrl_cpu": "0,1,2,3",
             "ai_cpu": "4,5,6,7", "aiv_num": 0, "hwts_frequency": "19.2", "aic_frequency": "680",
             "aiv_frequency": "1000"}]}

        with mock.patch('common_func.msprof_iteration.Utils.is_step_scene', return_value=True):
            check._ParseAiCpuData__analysis_ai_cpu(ai_cpu_datas)
            self.assertEqual(len(check.ai_cpu_datas), 1)

        with mock.patch(NAMESPACE + '.logging.warning'), \
             mock.patch('common_func.msprof_iteration.Utils.is_step_scene', return_value=True), \
             mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict',
                        return_value={1: 28017250105}):
            check = ParseAiCpuData(self.file_list, CONFIG)
            check._ParseAiCpuData__analysis_ai_cpu(wrong_ai_cpu_datas)
            self.assertEqual(check.ai_cpu_datas, [])

    def test_parse_ai_cpu(self):
        db_manager = DBManager()
        res = db_manager.create_table('ai_cpu.db')
        sql = "CREATE TABLE IF NOT EXISTS ai_cpu_datas(stream_id INTEGER,task_id INTEGER," \
              "sys_start INTEGER,sys_end INTEGER,node_name TEXT,compute_time REAL," \
              "memcpy_time REAL,task_time REAL,dispatch_time REAL,total_time REAL)"
        with mock.patch(NAMESPACE + '.AiStackDataCheckManager.contain_dp_aicpu_data',
                        return_value=True), \
             mock.patch(NAMESPACE + '.PathManager.get_db_path',
                        return_value='test\\db'), \
             mock.patch(NAMESPACE + '.DBManager.create_connect_db',
                        return_value=res), \
             mock.patch(NAMESPACE + '.DBManager.sql_create_general_table',
                        return_value=sql), \
             mock.patch(NAMESPACE + '.get_data_dir_sorted_files',
                        return_value=['DATA_PREPROCESS.AICPU.7.slice_0']), \
             mock.patch(NAMESPACE + '.is_valid_original_data',
                        return_value=True), \
             mock.patch(NAMESPACE + '.ParseAiCpuData._ParseAiCpuData__read_and_analysis_ai_cpu'), \
             mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'), \
             mock.patch(NAMESPACE + '.ParseAiCpuData._ParseAiCpuData__insert_data'), \
             mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 10}):
            check = ParseAiCpuData(self.file_list, CONFIG)
            check.parse_ai_cpu()
        with mock.patch(NAMESPACE + '.AiStackDataCheckManager.contain_dp_aicpu_data',
                        return_value=False), \
             mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 10}):
            check = ParseAiCpuData(self.file_list, CONFIG)
            result = check.parse_ai_cpu()
        self.assertEqual(result, None)
        res[1].execute("drop table ai_cpu_datas")
        db_manager.destroy(res)

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.ParseAiCpuData.parse_ai_cpu',
                        side_effect=OSError), \
             mock.patch(NAMESPACE + '.logging.error'), \
             mock.patch('common_func.msprof_iteration.MsprofIteration.get_iteration_end_dict', return_value={1: 10}):
            ParseAiCpuData(self.file_list, CONFIG).ms_run()
