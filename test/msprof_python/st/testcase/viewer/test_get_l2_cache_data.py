import unittest
from unittest import mock
from common_func.db_name_constant import DBNameConstant
from common_func.msvp_constant import MsvpConstant
from sqlite.db_manager import DBManager
from viewer.get_l2_cache_data import get_l2_cache_data, modify_l2_cache_headers, add_op_name

NAMESPACE = 'viewer.get_l2_cache_data'
columns = ['model_name', 'model_id', 'device_id']


class TestL2Cache(unittest.TestCase):

    def test_get_l2_cache_data_1(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=False):
            res = get_l2_cache_data('', "GELoad", 0, columns)
        self.assertEqual(res, MsvpConstant.MSVP_EMPTY_DATA)

    def test_get_l2_cache_data_2(self):
        create_sql = "CREATE TABLE IF NOT EXISTS GELoad" \
                     "(modelname, model_id, device_id, replayid, load_start, load_end, fusion_name, fusion_op_nums, " \
                     "op_names, stream_id, memory_input, memory_output, memory_weight, memory_workspace, " \
                     "memory_total, task_num, task_ids)"
        data = (("resnet50", 1, 0, 0, 118433101720, 118540073322, "conv1conv1_relu", 4,
                 "scale_conv1,bn_conv1,conv1,conv1_relu", 5, 1706208, 1605664, 100544, 0, 3412416, 1, 4),)
        insert_moudel_sql = "insert into {0} values ({value})".format(
            "GELoad", value="?," * (len(data[0]) - 1) + "?")
        db_manager = DBManager()
        test_sql = db_manager.create_table(DBNameConstant.DB_GE_MODEL_INFO, create_sql, insert_moudel_sql, data)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=test_sql), \
                mock.patch(NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.get_filtered_table_headers', return_value=columns), \
                mock.patch(NAMESPACE + '.GetTableData.get_table_data_for_device', return_value=[]), \
                mock.patch(NAMESPACE + '.modify_l2_cache_headers'):
            res = get_l2_cache_data('', "GELoad", 0, columns)
        self.assertEqual(res[2], 0)
        test_sql = db_manager.connect_db(DBNameConstant.DB_GE_MODEL_INFO)
        (test_sql[1]).execute("drop Table GELoad")
        db_manager.destroy(test_sql)

    def test_modify_l2_cache_headers(self):
        headers = ["r08", "r0_a", "r09", "0xb", "0xc", "0xd", "0x54", "0x55"]
        modify_l2_cache_headers(headers)

    def test_add_op_name_1(self):
        l2_cache_header = ["Task Id", "Stream Id"]
        l2_cache_data = [[1, 2], [2, 3]]
        op_dict = {'1-2': 1,
                   '2-3': 2}
        res = add_op_name(l2_cache_header, l2_cache_data, op_dict)
        self.assertEqual(res, True)

    def test_add_op_name_2(self):
        l2_cache_header = {"Task": 0, "Stream": 1}
        l2_cache_data = [[1, 2], [2, 3]]
        op_dict = {}
        res = add_op_name(l2_cache_header, l2_cache_data, op_dict)
        self.assertEqual(res, False)


if __name__ == '__main__':
    unittest.main()
