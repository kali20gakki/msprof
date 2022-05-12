import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from sqlite.db_manager import DBManager
from common_func.info_conf_reader import InfoConfReader
import sqlite3

from sqlite.db_manager import DBOpen
from tuning.data_manager import DataManager
from constant.constant import CONFIG, INFO_JSON

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}
NAMESPACE = 'tuning.data_manager'


class TestDataManager(unittest.TestCase):

    def test_get_data_by_infer_id(self):
        project_path = 'home\\data_manager'
        device_id = 123
        infer_id = 456
        headers = ['device_id', 'infer_id', 'project_path']
        datas = [[0, 0, 'home\\data_manager']]
        check = [{'device_id': 0, 'infer_id': 0, 'project_path': 'home\\data_manager'}]
        with mock.patch(NAMESPACE + '.DataManager.select_memory_workspace', return_value=[5, 4, 0]), \
             mock.patch(NAMESPACE + '.DataManager._get_base_data', return_value=(headers, datas)), \
             mock.patch(NAMESPACE + '.DataManager._get_extend_data'), \
             mock.patch(NAMESPACE + '.DataManager.get_memory_workspace'):
            key = DataManager()
            result = key.get_data_by_infer_id(project_path, device_id, infer_id)
        self.assertEqual(result, check)

    def test_get_base_data_1(self):
        device_id = 123
        infer_id = 456
        project_path = 'home\\data_manager'
        headers = ['device_id', 'infer_id', 'project_path']
        data = [[0, 0, 'home\\data_manager']]
        with mock.patch(NAMESPACE + '.DataManager.is_network', return_value=True), \
             mock.patch(NAMESPACE + '.GetExportDataConfigs.get_data_headers',
                        return_value=['device_id', 'infer_id']), \
             mock.patch(NAMESPACE + '.PathManager.get_db_path', return_value='data\\ai_core_op_summary'), \
             mock.patch(NAMESPACE + '.AiCoreOpReport.get_ai_core_op_summary_data', return_value=(headers, data, 0)):
            result = DataManager._get_base_data(device_id, infer_id, project_path)
        self.assertEqual(result, (headers, data))
        with mock.patch(NAMESPACE + '.DataManager.is_network', return_value=False), \
             mock.patch('os.path.join', return_value=True), \
             mock.patch(NAMESPACE + '.generate_config', return_value=sample_config), \
             mock.patch(NAMESPACE + '.get_task_based_core_data', return_value=(False, False, 0)), \
             mock.patch(NAMESPACE + '.get_core_sample_data', return_value=(1, 2, 0)):
            result = DataManager._get_base_data(device_id, infer_id, project_path)
        self.assertEqual(result, (1, 2))

    def test_get_base_data_2(self):
        device_id = 123
        infer_id = 456
        project_path = 'home\\data_manager'
        sample_config["ai_core_profiling_mode"] = "sample-based"
        sample_config["aiv_profiling_mode"] = "task-based"
        with mock.patch(NAMESPACE + '.DataManager.is_network', return_value=False), \
             mock.patch('os.path.join', return_value=True), \
             mock.patch(NAMESPACE + '.generate_config', return_value=sample_config), \
             mock.patch(NAMESPACE + '.get_core_sample_data', return_value=(False, False, 0)), \
             mock.patch(NAMESPACE + '.get_task_based_core_data', return_value=(1, 2, 0)):
            result = DataManager._get_base_data(device_id, infer_id, project_path)
        self.assertEqual(result, (1, 2))

    def test_get_extend_data(self):
        operator_dict = {}
        with mock.patch(NAMESPACE + '.DataManager.get_core_number'), \
             mock.patch(NAMESPACE + '.DataManager.get_vector_bound'):
            result = DataManager._get_extend_data(operator_dict)
        self.assertEqual(result, {})

    def test_get_vector_bound(self):
        extend_data_dict = {"vector_bound": 1, "mte2_ratio": 2, "mac_ratio": 3}
        operator_dict = {"vec_ratio": 4, "mte2_ratio": 5, "mac_ratio": 6}
        DataManager.get_vector_bound(extend_data_dict, operator_dict)

    def test_get_core_number(self):
        extend_data_dict = {"core_num": None}
        InfoConfReader()._info_json = INFO_JSON
        DataManager.get_core_number(extend_data_dict)

    def test_get_memory_workspace_1(self):
        memory_workspaces = [[1, '123', 3]]
        operator_dict = {"stream_id": 1, "task_id": '123', "memory_workspace": 2}
        DataManager.get_memory_workspace(memory_workspaces, operator_dict)

    def test_get_memory_workspace_2(self):
        memory_workspaces = None
        operator_dict = {"memory_workspace": 2}
        DataManager.get_memory_workspace(memory_workspaces, operator_dict)

    def test_select_memory_workspace(self):
        project = 123
        device_id = 0
        create_sql = "create table IF NOT EXISTS GELoad (modelname TEXT,model_id INTEGER,device_id INTEGER," \
                     "replayid INTEGER,load_start INTEGER,load_end INTEGER,fusion_name TEXT," \
                     "fusion_op_nums INTEGER,op_names TEXT,stream_id INTEGER,memory_input INTEGER," \
                     "memory_output INTEGER,memory_weight INTEGER,memory_workspace INTEGER," \
                     "memory_total INTEGER,task_num INTEGER,task_ids TEXT)"
        data = [('resnet50', 1, 0, 0, 960265000000, 960593000000, 'conv1', 3, 'bn_conv1, conv1, scale_conv1',
                 5, 1706208, 1605664, 100544, 0, 3412416, 1, 4)]
        insert_sql = "insert into {0} values ({value})".format("GELoad", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        res = db_manager.create_table("ge_model_info.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DataManager.is_network', return_value=False):
            result = DataManager.select_memory_workspace(project, device_id)
        self.assertEqual(result, [])
        db_manager = DBManager()
        res = db_manager.create_table("ge_model_info.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DataManager.is_network', return_value=True), \
             mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(res[0], res[1])), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True):
            result = DataManager.select_memory_workspace(project, device_id)
        self.assertEqual(result, [])
        db_manager = DBManager()
        res = db_manager.create_table("ge_model_info.db", create_sql, insert_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            DataManager.select_memory_workspace(project, device_id)
        res[1].execute("drop table GELoad")
        db_manager.destroy(res)

    def test_is_network(self):
        project = 123
        device_id = 0
        create_sql = "create table IF NOT EXISTS acl_data (api_name text,api_type text,start_time INTEGER," \
                     "end_time INTEGER,process_id INTEGER,thread_id INTEGER,device_id INTEGER)"
        data = [(None, None, 117962568517, 118405906350, 2244, 2246, None)]
        with DBOpen(DBNameConstant.DB_ACL_MODULE) as db_open:
            db_open.create_table(create_sql)
            db_open.insert_data(DBNameConstant.TABLE_ACL_DATA, data)
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(db_open.db_conn, db_open.db_curs)):
                result = DataManager.is_network(project, device_id)
            self.assertEqual(result, True)

            curs = mock.Mock()
            curs.execute.side_effect = sqlite3.Error
            with mock.patch(NAMESPACE + '.DBManager.check_connect_db', return_value=(curs, curs)), \
                 mock.patch(NAMESPACE + '.logging.error'):
                DataManager.is_network(project, device_id)

            with mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
                DataManager.is_network(project, device_id)


if __name__ == '__main__':
    unittest.main()
