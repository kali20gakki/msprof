import sqlite3
import unittest
from argparse import Namespace
from unittest import mock

import pytest
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.common import print_info, warn, error
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_exception import ProfException
from msinterface.msprof_export import ExportCommand
from sqlite.db_manager import DBManager

NAMESPACE = 'msinterface.msprof_export'


def create_trace_db():
    create_ge_sql = "create table if not exists TaskInfo (device_id INTEGER," \
                    "model_name TEXT,model_id INTEGER,op_name TEXT,stream_id INTEGER," \
                    "task_id INTEGER,block_dim INTEGER,op_state TEXT,task_type TEXT," \
                    "op_type TEXT,iter_id INTEGER,input_count INTEGER,input_formats TEXT," \
                    "input_data_types TEXT,input_shapes TEXT,output_count INTEGER," \
                    "output_formats TEXT,output_data_types TEXT,output_shapes TEXT)"
    insert_ge_sql = "insert into {} values (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)".format('TaskInfo')
    data = ((0, 'test', 1, 'model', 5, 3, 2, 'static', 'AI_CORE', 'trans_data',
             1, 1, '12', '1750', '1752', 0, 'test', 'test2', 'test3'),)
    db_manager = DBManager()
    res = db_manager.create_table("ge_info.db", create_ge_sql, insert_ge_sql, data)
    res[0].close()

    create_sql = "CREATE TABLE IF NOT EXISTS StepTrace (index_id INT,model_id INT,step_start REAL," \
                 " step_end INT,iter_id INT,ai_core_num INT)"
    insert_sql = "insert into {} values (?,?,?,?,?,?)".format('StepTrace')
    data = ((1, 1, 2, 3, 4, 5),)
    db_manager = DBManager()
    res = db_manager.create_table("step_trace.db", create_sql, insert_sql, data)
    res[0].close()

    return db_manager


class DtoData:

    def __init__(self, *args):
        filed = args[0]
        self.iter_id = filed[0]
        self.model_id = filed[1]
        self.index_id = filed[2]


class TestExportCommand(unittest.TestCase):

    def tearDown(self) -> None:
        _db_manager = DBManager()
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_STEP_TRACE))
        _db_manager.destroy(_db_manager.create_sql(DBNameConstant.DB_TRACE))

    def test_add_export_type(self):
        args_dic = {"collection_path": "test", "iteration_id": 1, "model_id": None}
        args = Namespace(**args_dic)
        sample_config = {"llc_profiling": ""}
        with mock.patch(NAMESPACE + ".DataCheckManager.check_data_exist", return_value=True), \
                mock.patch(NAMESPACE + ".ConfigMgr.read_sample_config", return_value=sample_config):
            test = ExportCommand("timeline", args)
            test.sample_config = {'devices': '0'}
            test._add_export_type("")
        with mock.patch(NAMESPACE + ".DataCheckManager.check_data_exist", return_value=True), \
                mock.patch(NAMESPACE + ".ConfigMgr.read_sample_config", return_value=sample_config):
            test = ExportCommand("summary", args)
            test.sample_config = {'devices': '0'}
            test._add_export_type("")

    def test_check_model_id_1(self):
        args_dic = {"collection_path": "test", "iteration_id": 2, "model_id": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + ".ExportCommand.get_model_id_set", return_value={1}), \
                mock.patch('analyzer.scene_base.profiling_scene.Utils.get_scene',
                           return_value="step_info"):
            ProfilingScene().init('')
            test = ExportCommand("timeline", args)
            test._check_model_id("")

    def test_check_model_id_2(self):
        db_manager = create_trace_db()
        args_dic = {"collection_path": "test", "iteration_id": 2, "model_id": None}
        args = Namespace(**args_dic)
        with mock.patch('analyzer.scene_base.profiling_scene.Utils.get_scene',
                        return_value="train"):
            ProfilingScene().init('')
            test = ExportCommand("timeline", args)
            test._check_model_id(db_manager.db_path + "/../")

    def test_analyse_sample_config(self):
        args_dic = {"collection_path": "test", "iteration_id": 1, "model_id": 3}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + '.ConfigMgr.read_sample_config', return_value=1):
            test = ExportCommand("timeline", args)
            test._analyse_sample_config("")
        self.assertEqual(test.sample_config, 1)

    def test_analyse_data(self):
        args_dic = {"collection_path": "test", "iteration_id": 1, "model_id": 3}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + ".FileManager.is_analyzed_data", return_value=False), \
                mock.patch(NAMESPACE + ".analyze_collect_data"):
            test = ExportCommand("timeline", args)
            test._analyse_data("")

        with mock.patch(NAMESPACE + ".FileManager.is_analyzed_data", return_value=True), \
                mock.patch(NAMESPACE + ".print_info"):
            test = ExportCommand("timeline", args)
            test._analyse_data("")

    def test_check_index_id_range_1(self):
        args_dic = {"collection_path": "test", "iteration_id": 2, "model_id": None, "iteration_count": 1}
        args = Namespace(**args_dic)
        cur = mock.Mock()
        cur.execute.side_effect = sqlite3.Error
        with mock.patch(NAMESPACE + ".DBManager.destroy_db_connect"):
            test = ExportCommand("timeline", args)
            test._check_index_id("")

    def test__check_index_id_range_2(self):
        args_dic = {"collection_path": "test", "iteration_id": 1, "model_id": None, "iteration_count": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + ".Utils.get_scene", return_value=Constant.SINGLE_OP):
            ProfilingScene()._scene = Constant.SINGLE_OP
            test = ExportCommand("timeline", args)
            test._check_index_id("")

    def test_check_index_id_given_invalid_iter_id_when_iter_id_less_than_zero_then_raise_exception(self):
        args_dic = {"collection_path": "test", "iteration_id": -1, "model_id": None, "iteration_count": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + ".Utils.get_scene", return_value=Constant.SINGLE_OP):
            with pytest.raises(ProfException) as err:
                ProfilingScene()._scene = Constant.SINGLE_OP
                test = ExportCommand("timeline", args)
                test._check_index_id("")
            self.assertEqual(ProfException.PROF_INVALID_STEP_TRACE_ERROR, err.value.code)

    def test_prepare_for_export(self):
        args_dic = {"collection_path": "test", "iteration_id": 3, "model_id": 3, "iteration_count": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + ".ExportCommand._analyse_sample_config"), \
                mock.patch(NAMESPACE + ".ExportCommand._analyse_data"), \
                mock.patch(NAMESPACE + ".ExportCommand._add_export_type"), \
                mock.patch(NAMESPACE + ".ExportCommand._check_index_id"), \
                mock.patch(NAMESPACE + ".ExportCommand._check_model_id"), \
                mock.patch("msinterface.msprof_timeline" + ".MsprofIteration.get_iteration_time",
                           return_value=100), \
                mock.patch(NAMESPACE + ".PathManager.get_summary_dir", return_value=""), \
                mock.patch(NAMESPACE + ".check_path_valid"), \
                mock.patch(NAMESPACE + ".print_info"):
            InfoConfReader()._info_json = {"devices": 1}
            test = ExportCommand("timeline", args)
            with mock.patch(NAMESPACE + ".LoadInfoManager.load_info"), \
                    mock.patch('framework.file_dispatch.FileDispatch.dispatch_calculator'):
                test._prepare_for_export("")

    def test_handle_export_data(self):
        args_dic = {"collection_path": "test", "iteration_id": 3, "model_id": 3, "iteration_count": 1}
        args = Namespace(**args_dic)
        result = {"status": 0, "data": [], "info": ""}
        with mock.patch(NAMESPACE + ".MsProfExportDataUtils.export_data"), \
                mock.patch(NAMESPACE + ".json.loads", return_value=result), \
                mock.patch(NAMESPACE + ".ExportCommand._print_export_info"):
            test = ExportCommand("timeline", args)
            test._handle_export_data("")

        result["status"] = 1
        with mock.patch(NAMESPACE + ".MsProfExportDataUtils.export_data"), \
                mock.patch(NAMESPACE + ".json.loads", return_value=result), \
                mock.patch(NAMESPACE + ".error"):
            test = ExportCommand("timeline", args)
            test._handle_export_data("")

        result["status"] = 2
        with mock.patch(NAMESPACE + ".MsProfExportDataUtils.export_data"), \
                mock.patch(NAMESPACE + ".json.loads", return_value=result), \
                mock.patch(NAMESPACE + ".warn"):
            test = ExportCommand("timeline", args)
            test._handle_export_data("")

    def test_print_export_info(self):
        args_dic = {"collection_path": "test", "iteration_id": 3, "model_id": 1, "iteration_count": 1}
        args = Namespace(**args_dic)
        params = {"data_type": "nic", "device_id": 0, "iter_id": 1}
        with mock.patch(NAMESPACE + '.print_info'):
            test = ExportCommand("timeline", args)
            test._print_export_info(params, [])

    def test_handle_export(self):
        args_dic = {"collection_path": "test", "iteration_id": 3, "model_id": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + ".prepare_for_parse"), \
                mock.patch(NAMESPACE + ".ExportCommand._export_data"), \
                mock.patch(NAMESPACE + ".ExportCommand._prepare_for_export"):
            test = ExportCommand("timeline", args)
            test.list_map = {'export_type_list': ['acl'], 'devices_list': []}
            test._handle_export("")
            test.list_map = {'export_type_list': ['acl'], 'devices_list': [1]}
            test._handle_export("")

    def test_show_tuning_result(self):
        args_dic = {"collection_path": "test", "iteration_id": 3, "model_id": 1, "iteration_count": 1}
        args = Namespace(**args_dic)
        test = ExportCommand("timeline", args)
        test._show_tuning_result('')

        with mock.patch(NAMESPACE + '.ProfilingTuning.run', return_value=(True,)), \
                mock.patch(NAMESPACE + ".TuningView.show_by_dev_id"):
            test = ExportCommand("summary", args)
            test.list_map["devices_list"] = ["1"]
            test._show_tuning_result('')

    def test_process(self):
        args_dic = {"collection_path": "test", "iteration_id": 3, "model_id": 1, "iteration_count": 1}
        args = Namespace(**args_dic)
        with mock.patch('os.path.join', return_value='JOB/device_0'), \
                mock.patch('os.path.realpath', return_value='JOB/device_0'), \
                mock.patch('os.listdir', return_value=[]), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.ExportCommand._handle_export'), \
                mock.patch(NAMESPACE + '.ExportCommand._show_tuning_result'), \
                mock.patch(NAMESPACE + '.ExportCommand._process_sub_dirs'):
            with mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', retrun_value=True):
                test = ExportCommand("summary", args)
                test.list_map["devices_list"] = ["1"]
                test.process()
            with mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', retrun_value=False):
                test = ExportCommand("summary", args)
                test.list_map["devices_list"] = ["1"]
                test.process()

    def test_process_1(self):
        args_dic = {"collection_path": "test", "iteration_id": 3, "model_id": 1, "iteration_count": 1}
        args = Namespace(**args_dic)
        json_data_result = (False, True)
        path_dir = (['device_0'], ['host'], ['device_1'])
        with mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', side_effect=json_data_result), \
                mock.patch(NAMESPACE + '.MsprofJobSummary.export'):
            with mock.patch(NAMESPACE + '.get_path_dir', side_effect=path_dir), \
                    mock.patch('os.path.join', return_value='JOB/device_0'), \
                    mock.patch('os.path.realpath', return_value='JOB/device_0'), \
                    mock.patch('os.listdir', return_value=[]), \
                    mock.patch(NAMESPACE + '.check_path_valid'), \
                    mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', side_effect=json_data_result), \
                    mock.patch(NAMESPACE + '.ExportCommand._handle_export'), \
                    mock.patch(NAMESPACE + '.ExportCommand._show_tuning_result'), \
                    mock.patch(NAMESPACE + '.warn'), \
                    mock.patch(NAMESPACE + '.MsprofJobSummary.export'):
                test = ExportCommand("summary", args)
                test.list_map["devices_list"] = ["1"]
                test.process()

    def test_handle_export_raise_profexception_0(self) -> None:
        args_dic = {"collection_path": "test", "iteration_id": 3, "model_id": 1, "iteration_count": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + '.ExportCommand._prepare_export', side_effect=ProfException(2, 'test', print_info)):
            test = ExportCommand("summary", args)
            test._handle_export('test')

    def test_handle_export_raise_profexception_1(self) -> None:
        args_dic = {"collection_path": "test", "iteration_id": 3, "model_id": 1, "iteration_count": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + '.ExportCommand._prepare_export', side_effect=ProfException(2, 'test', warn)):
            test = ExportCommand("summary", args)
            test._handle_export('test')

    def test_handle_export_raise_profexception_2(self) -> None:
        args_dic = {"collection_path": "test", "iteration_id": 3, "model_id": 1, "iteration_count": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + '.ExportCommand._prepare_export', side_effect=ProfException(2)):
            test = ExportCommand("summary", args)
            test._handle_export('test')

    def test_handle_export_raise_profexception_3(self) -> None:
        args_dic = {"collection_path": "test", "iteration_id": 3, "model_id": 1, "iteration_count": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + '.ExportCommand._prepare_export'), \
                mock.patch(NAMESPACE + '.ExportCommand._export_data', side_effect=ProfException(2, 'test', error)):
            test = ExportCommand("summary", args)
            test.list_map = {'export_type_list': [1]}
            test._handle_export('test')

    def test_handle_export_raise_profexception_4(self) -> None:
        args_dic = {"collection_path": "test", "iteration_id": 3, "model_id": 1, "iteration_count": 1}
        args = Namespace(**args_dic)
        with mock.patch(NAMESPACE + '.ExportCommand._prepare_export'), \
                mock.patch(NAMESPACE + '.ExportCommand._export_data', side_effect=ProfException(2)):
            test = ExportCommand("summary", args)
            test.list_map = {'export_type_list': [1]}
            test._handle_export('test')

    def test_handle_export_raise_profexception_5(self) -> None:
        args_dic = {"collection_path": "test", "iteration_id": 3, "model_id": 1, "iteration_count": 1}
        args = Namespace(**args_dic)
        test = ExportCommand("summary", args)
        test.list_map = {'export_type_list': [1]}
        test._handle_export('test')

    def test_handle_export_raise_profexception_6(self) -> None:
        InfoConfReader()._info_json = {}
        args_dic = {"collection_path": "test", "iteration_id": 3, "model_id": 1, "iteration_count": 1}
        args = Namespace(**args_dic)
        with mock.patch('os.path.exists', return_value=True), \
                mock.patch('common_func.msprof_common.check_dir_writable'), \
                mock.patch('common_func.msprof_common.check_free_memory'), \
                mock.patch('os.listdir', return_value=['test']):
            test = ExportCommand("summary", args)
            test.list_map = {'export_type_list': [1]}
            test._handle_export('test')


if __name__ == '__main__':
    unittest.main()
