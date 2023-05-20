#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from mscalculate.cann.cann_event_generator import CANNEventGenerator
from sqlite.db_manager import DBManager

NAMESPACE = 'mscalculate.cann.cann_event_generator'


class TestClusterLinkCalculate(unittest.TestCase):

    def test_run(self):
        with mock.patch(NAMESPACE + '.CANNEventGenerator.generate_api_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_node_basic_info_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_tensor_info_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_task_track_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_mem_copy_info_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_graph_id_map_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_fusion_op_info_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_ctx_id_event'), \
                mock.patch(NAMESPACE + '.CANNEventGenerator.generate_hccl_info_event'):
            ret = CANNEventGenerator(["test"], set()).run()
            self.assertEqual(ret.queues, {})

    def test_collect_time_period_data(self):
        api_db_manager = DBManager()
        api_sql = "create table if not exists ApiData (struct_type TEXT," \
                  "id TEXT,level TEXT,thread_id INTEGER,item_id TEXT," \
                  "start INTEGER,end INTEGER)"
        insert_api_sql = "insert into {} values (?,?,?,?,?,?,?)".format('ApiData')
        api_data = ((0, 0, 0, 0, 0, 0, 0),)
        event_sql = "create table if not exists EventData (struct_type TEXT," \
                    "level TEXT,thread_id INTEGER,item_id TEXT," \
                    "request_id INTEGER,timestamp INTEGER)"
        insert_event_sql = "insert into {} values (?,?,?,?,?,?)".format('EventData')
        event_data = ((0, 0, 0, 0, 0, 10), (0, 0, 0, 0, 0, 10))
        event_db_manager = DBManager()
        api_conn, api_cur = api_db_manager.create_table('api.db', api_sql, insert_api_sql, api_data)
        event_conn, event_cur = event_db_manager.create_table('event.db', event_sql, insert_event_sql, event_data)
        conn_tuple = ((api_conn, api_cur), (event_conn, event_cur))
        with mock.patch('msmodel.interface.base_model.DBManager.check_connect_db', side_effect=conn_tuple), \
                mock.patch('msmodel.interface.base_model.DBManager.create_connect_db', side_effect=conn_tuple):
            check = CANNEventGenerator("test", set())
            check.generate_api_event()
            self.assertEqual(len(check.event_queue.queues), 1)
            self.assertEqual(check.thread_set, {0})
        api_conn, api_cur = api_db_manager.create_table('api.db')
        event_conn, event_cur = event_db_manager.create_table('event.db')
        api_db_manager.destroy((api_conn, api_cur))
        event_db_manager.destroy((event_conn, event_cur))

    def test_generate_node_basic_info_event(self):
        node_db_manager = DBManager()
        node_sql = "create table if not exists NodeBasicInfo (level TEXT," \
                   "struct_type TEXT,thread_id TEXT,timestamp INTEGER,op_name TEXT," \
                   "task_type INTEGER,op_type INTEGER,block_dim INTEGER," \
                   "mix_block_dim INTEGER,op_flag INTEGER,is_dynamic INTEGER)"
        insert_node_sql = "insert into {} values (?,?,?,?,?,?,?,?,?,?,?)".format('NodeBasicInfo')
        node_data = ((0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),)
        node_conn, node_cur = node_db_manager.create_table('node_basic_info.db', node_sql, insert_node_sql, node_data)
        with mock.patch('msmodel.interface.base_model.DBManager.check_connect_db',
                        return_value=(node_conn, node_cur)), \
                mock.patch('msmodel.interface.base_model.DBManager.create_connect_db',
                           return_value=(node_conn, node_cur)):
            check = CANNEventGenerator("test", set())
            check.generate_node_basic_info_event()
            self.assertEqual(len(check.event_queue.queues), 1)
            self.assertEqual(check.thread_set, {'0'})
        node_conn, node_cur = node_db_manager.create_table('node_basic_info.db')
        node_db_manager.destroy((node_conn, node_cur))

    def test_generate_tensor_info_event(self):
        tensor_db_manager = DBManager()
        tensor_sql = "create table if not exists TensorInfoV2 (level TEXT," \
                     "struct_type TEXT,thread_id TEXT,timestamp INTEGER,op_name TEXT," \
                     "tensor_num INTEGER,input_formats INTEGER,input_data_types INTEGER," \
                     "input_shapes INTEGER,output_formats INTEGER,output_data_types INTEGER,output_shapes INTEGER)"
        insert_tensor_sql = "insert into {} values (?,?,?,?,?,?,?,?,?,?,?,?)".format('TensorInfoV2')
        tensor_data = ((0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),)
        tensor_conn, tensor_cur = tensor_db_manager.create_table('tensor_info.db', tensor_sql, insert_tensor_sql,
                                                                 tensor_data)
        with mock.patch('msmodel.interface.base_model.DBManager.check_connect_db',
                        return_value=(tensor_conn, tensor_cur)), \
                mock.patch('msmodel.interface.base_model.DBManager.create_connect_db',
                           return_value=(tensor_conn, tensor_cur)):
            check = CANNEventGenerator("test", set())
            check.generate_tensor_info_event()
            self.assertEqual(len(check.event_queue.queues), 1)
            self.assertEqual(check.thread_set, {'0'})
        tensor_conn, tensor_cur = tensor_db_manager.create_table('tensor_info.db')
        tensor_db_manager.destroy((tensor_conn, tensor_cur))

    def test_generate_task_track_event(self):
        track_db_manager = DBManager()
        track_sql = "create table if not exists TaskTrack (device_id TEXT," \
                    "timestamp TEXT,task_type TEXT,stream_id INTEGER,task_id TEXT," \
                    "thread_id INTEGER,batch_id INTEGER,struct_type INTEGER," \
                    "level INTEGER,data_len INTEGER)"
        insert_track_sql = "insert into {} values (?,?,?,?,?,?,?,?,?,?)".format('TaskTrack')
        track_data = ((0, 0, 0, 0, 0, 0, 0, 0, 0, 0),)
        track_conn, track_cur = track_db_manager.create_table('rts_track.db', track_sql, insert_track_sql,
                                                              track_data)
        with mock.patch('msmodel.interface.base_model.DBManager.check_connect_db',
                        return_value=(track_conn, track_cur)), \
                mock.patch('msmodel.interface.base_model.DBManager.create_connect_db',
                           return_value=(track_conn, track_cur)):
            check = CANNEventGenerator("test", set())
            check.generate_task_track_event()
            self.assertEqual(len(check.event_queue.queues), 1)
            self.assertEqual(check.thread_set, {0})
        track_conn, track_cur = track_db_manager.create_table('rts_track.db')
        track_db_manager.destroy((track_conn, track_cur))

    def test_generate_mem_copy_info_event(self):
        mem_db_manager = DBManager()
        mem_sql = "create table if not exists MemcpyInfo (struct_type TEXT," \
                  "level TEXT,thread_id TEXT,data_len INTEGER,timestamp TEXT," \
                  "data_size INTEGER,memcpy_direction INTEGER)"
        insert_mem_sql = "insert into {} values (?,?,?,?,?,?,?)".format('MemcpyInfo')
        mem_data = ((0, 0, 0, 0, 0, 0, 0),)
        mem_conn, mem_cur = mem_db_manager.create_table('runtime.db', mem_sql, insert_mem_sql,
                                                        mem_data)
        with mock.patch('msmodel.interface.base_model.DBManager.check_connect_db',
                        return_value=(mem_conn, mem_cur)), \
                mock.patch('msmodel.interface.base_model.DBManager.create_connect_db',
                           return_value=(mem_conn, mem_cur)):
            check = CANNEventGenerator("test", set())
            check.generate_mem_copy_info_event()
            self.assertEqual(len(check.event_queue.queues), 1)
            self.assertEqual(check.thread_set, {'0'})
        mem_conn, mem_cur = mem_db_manager.create_table('runtime.db')
        mem_db_manager.destroy((mem_conn, mem_cur))

    def test_generate_ctx_id_event(self):
        mem_db_manager = DBManager()
        mem_sql = "create table if not exists CtxId (struct_type TEXT," \
                  "level TEXT,thread_id TEXT,op_name INTEGER,timestamp TEXT," \
                  "ctx_id_num INTEGER,ctx_id INTEGER)"
        insert_mem_sql = "insert into {} values (?,?,?,?,?,?,?)".format('CtxId')
        mem_data = ((0, 0, 0, 0, 0, 0, 0),)
        mem_conn, mem_cur = mem_db_manager.create_table('ctx_id.db', mem_sql, insert_mem_sql,
                                                        mem_data)
        with mock.patch('msmodel.interface.base_model.DBManager.check_connect_db',
                        return_value=(mem_conn, mem_cur)), \
                mock.patch('msmodel.interface.base_model.DBManager.create_connect_db',
                           return_value=(mem_conn, mem_cur)):
            check = CANNEventGenerator("test", set())
            check.generate_ctx_id_event()
            self.assertEqual(len(check.event_queue.queues), 1)
            self.assertEqual(check.thread_set, {'0'})
        mem_conn, mem_cur = mem_db_manager.create_table('ctx_id.db')
        mem_db_manager.destroy((mem_conn, mem_cur))

    def test_generate_hccl_info_event(self):
        mem_db_manager = DBManager()
        mem_sql = "create table if not exists HcclInfoData (struct_type TEXT," \
                  "level TEXT,thread_id TEXT,op_name INTEGER,timestamp TEXT," \
                  "ctx_id_num INTEGER,ctx_id INTEGER)"
        insert_mem_sql = "insert into {} values (?,?,?,?,?,?,?)".format('HcclInfoData')
        mem_data = ((0, 0, 0, 0, 0, 0, 0),)
        mem_conn, mem_cur = mem_db_manager.create_table('hccl_info.db', mem_sql, insert_mem_sql,
                                                        mem_data)
        with mock.patch('msmodel.interface.base_model.DBManager.check_connect_db',
                        return_value=(mem_conn, mem_cur)), \
                mock.patch('msmodel.interface.base_model.DBManager.create_connect_db',
                           return_value=(mem_conn, mem_cur)):
            check = CANNEventGenerator("test", set())
            check.generate_hccl_info_event()
            self.assertEqual(len(check.event_queue.queues), 1)
            self.assertEqual(check.thread_set, {'0'})
        mem_conn, mem_cur = mem_db_manager.create_table('hccl_info.db')
        mem_db_manager.destroy((mem_conn, mem_cur))

    def test_generate_graph_id_map_event(self):
        mem_db_manager = DBManager()
        mem_sql = "create table if not exists GraphIdMap (struct_type TEXT," \
                  "level TEXT,thread_id TEXT,op_name INTEGER,timestamp TEXT," \
                  "model_name INTEGER,graph_id INTEGER)"
        insert_mem_sql = "insert into {} values (?,?,?,?,?,?,?)".format('GraphIdMap')
        mem_data = ((0, 0, 0, 0, 0, 0, 0),)
        mem_conn, mem_cur = mem_db_manager.create_table('graph_id_map.db', mem_sql, insert_mem_sql,
                                                        mem_data)
        with mock.patch('msmodel.interface.base_model.DBManager.check_connect_db',
                        return_value=(mem_conn, mem_cur)), \
                mock.patch('msmodel.interface.base_model.DBManager.create_connect_db',
                           return_value=(mem_conn, mem_cur)):
            check = CANNEventGenerator("test", set())
            check.generate_graph_id_map_event()
            self.assertEqual(len(check.event_queue.queues), 1)
            self.assertEqual(check.thread_set, {'0'})
        mem_conn, mem_cur = mem_db_manager.create_table('graph_id_map.db')
        mem_db_manager.destroy((mem_conn, mem_cur))

    def test_generate_fusion_op_info_event(self):
        mem_db_manager = DBManager()
        mem_sql = "create table if not exists FusionOPInfo (struct_type TEXT," \
                  "level TEXT,thread_id TEXT,op_name INTEGER,timestamp TEXT," \
                  "fusion_op_num INTEGER,memory_input INTEGER,memory_output INTEGER," \
                  "memory_weight INTEGER,memory_workspace INTEGER,memory_total INTEGER," \
                  "fusion_op_names INTEGER)"
        insert_mem_sql = "insert into {} values (?,?,?,?,?,?,?,?,?,?,?,?)".format('FusionOPInfo')
        mem_data = ((0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),)
        mem_conn, mem_cur = mem_db_manager.create_table('fusion_op_info.db', mem_sql, insert_mem_sql,
                                                        mem_data)
        with mock.patch('msmodel.interface.base_model.DBManager.check_connect_db',
                        return_value=(mem_conn, mem_cur)), \
                mock.patch('msmodel.interface.base_model.DBManager.create_connect_db',
                           return_value=(mem_conn, mem_cur)):
            check = CANNEventGenerator("test", set())
            check.generate_fusion_op_info_event()
            self.assertEqual(len(check.event_queue.queues), 1)
            self.assertEqual(check.thread_set, {'0'})
        mem_conn, mem_cur = mem_db_manager.create_table('fusion_op_info.db')
        mem_db_manager.destroy((mem_conn, mem_cur))


if __name__ == '__main__':
    unittest.main()
