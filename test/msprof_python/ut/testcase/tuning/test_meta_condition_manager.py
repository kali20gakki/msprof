import unittest
from unittest import mock

from tuning.meta_condition_manager import MetaConditionManager
from tuning.meta_condition_manager import NetConditionManager
from tuning.meta_condition_manager import OperatorConditionManager
from tuning.meta_condition_manager import get_two_decimal
from tuning.meta_condition_manager import load_condition_files

sample_config = {"model_id": 1, 'iter_id': 'dasfsd', 'result_dir': 'jasdfjfjs',
                 "ai_core_profiling_mode": "task-based", "aiv_profiling_mode": "sample-based"}
NAMESPACE = 'tuning.meta_condition_manager'


class TestMetaConditionManager(unittest.TestCase):

    def test_load_condition_files(self):
        key = MetaConditionManager()
        load_condition_files()
        self.assertEqual(len(key.conditions), 27)

    def test_format_operator(self):
        expression = r'\+-|\+\+|--|-\+'
        with mock.patch(NAMESPACE + '.load_condition_files'):
            result = MetaConditionManager._MetaConditionManager__format_operator(expression)
        self.assertEqual(result, r'\-|\+\+|+|-\+')

    def test_calculate_expression(self):
        expression = '(1+2)'
        with mock.patch(NAMESPACE + '.MetaConditionManager._MetaConditionManager__format_operator',
                        return_value=expression), \
             mock.patch(NAMESPACE + '.load_condition_files'), \
             mock.patch(NAMESPACE + '.MetaConditionManager.calculate_basic_expression', return_value='+'):
            result = MetaConditionManager.calculate_expression(expression)
        self.assertEqual(result, '+')

    def test_calculate_basic_expression_1(self):
        combination_0 = [['120*12', '*', '1440.0'], ['12/1', '/', '12.0'], ['1%1', '%', '0.0'], ['1&1', '&', '1']]
        for i in combination_0:
            result = MetaConditionManager.calculate_basic_expression(i[0])
            self.assertEqual(float(result), float(i[2]))

    def test_calculate_basic_expression_2(self):
        combination_1 = [['0+0', '+'], ['0-0', '-']]
        for j in combination_1:
            result = MetaConditionManager.calculate_basic_expression(j[0])
            self.assertEqual(float(result), float('0.0'))

    def test_cal_formula_condition(self):
        operator_data = {"lefty": 1, "op_name": 2}
        condition = {"left": ("lefty",), "formula": ">", "right": 2, "cmp": "<"}
        with mock.patch(NAMESPACE + '.MetaConditionManager.calculate_expression', return_value=1), \
             mock.patch(NAMESPACE + '.load_condition_files'):
            result = MetaConditionManager.cal_formula_condition(operator_data, condition, 'op_name')
        self.assertEqual(result, [2])

    def test_cal_normal_condition(self):
        operator_data = {"lefty": 2, "righty": 1, "op_name": '123'}
        condition = {"cmp": ">", "left": "lefty", "right": 0}
        with mock.patch(NAMESPACE + '.load_condition_files'):
            result = MetaConditionManager.cal_normal_condition(operator_data, condition, 'op_name')
        self.assertEqual(result, ['123'])

    def test_merge_set(self):
        condition_id = 'asd.sdd &&  ssss'
        condition_id_dict = {'324': 1}
        with mock.patch(NAMESPACE + '.re.sub', return_value='&&'), \
             mock.patch(NAMESPACE + '.load_condition_files'), \
             mock.patch(NAMESPACE + '.time.time', return_value=324):
            key = MetaConditionManager()
            result = key.merge_set(condition_id, condition_id_dict)
        self.assertEqual(result, '324')

    def test_cal_conditions(self):
        operator_data = {}
        condition_id = '(+)'
        with mock.patch(NAMESPACE + '.MetaConditionManager.cal_normal_condition', return_value=['123']), \
             mock.patch(NAMESPACE + '.MetaConditionManager.cal_formula_condition', return_value=[2]), \
             mock.patch(NAMESPACE + '.load_condition_files'), \
             mock.patch(NAMESPACE + '.MetaConditionManager.merge_set', return_value='+'):
            key = MetaConditionManager()
            key.conditions = {'+': {'type': 'normal'}, '1': {'type': 'formula'}}
            result = key.cal_conditions(operator_data, condition_id, 'Op Summary')
        self.assertEqual(result, ['123'])

    def test_cal_condition_1(self):
        operator_data = {}
        condition_id = '(+)'
        condition_id_1 = '(1+2)'
        condition_id_2 = '(2+1)'
        tag_key = 'op_name'
        with mock.patch(NAMESPACE + '.load_condition_files'):
            key = MetaConditionManager()
            key.conditions = {'(+)': {'type': 'normal'}, '(1+2)': {'type': 'formula', 'left': [1, 2]},
                              '(2+1)': {'type': 'count'}}
            result = key.cal_condition(operator_data, condition_id, tag_key)
            result_1 = key.cal_condition(operator_data, condition_id_1, tag_key)
            result_2 = key.cal_condition(operator_data, condition_id_2, tag_key)
        self.assertEqual(result, [])
        self.assertEqual(result_1, [])
        self.assertEqual(result_2, None)

    def test_cal_condition_error(self):
        operator_data = {}
        condition_id = '(+)'
        with mock.patch(NAMESPACE + '.logging.error'), \
                mock.patch('common_func.file_manager.check_path_valid'):
            key = MetaConditionManager()
            key.conditions = {'(+)': {'type': None}}
            result = key.cal_condition(operator_data, condition_id, 'op_name')
            self.assertEqual(result, [])

    def test_get_condition(self):
        condition_ids = ['1']
        with mock.patch(NAMESPACE + '.load_condition_files'):
            key = MetaConditionManager()
            key.conditions = {'1': 'one'}
            result = key.get_condition(condition_ids)
        self.assertEqual(result, ['one'])


class TestOperatorConditionManager(unittest.TestCase):

    def test_cal_count_condition(self):
        operator_data_list = ()
        condition = '123'
        with mock.patch(NAMESPACE + '.load_condition_files'):
            key = OperatorConditionManager()
            result = key.cal_count_condition(operator_data_list, condition, 'op_name')
        self.assertEqual(result, [])


class TestNetConditionManager(unittest.TestCase):

    def test_cal_count_condition(self):
        operator_data_list = []
        condition = {'dependency': '(+)', 'threshold': '1234', 'cmp': '>'}
        with mock.patch(NAMESPACE + '.NetConditionManager.cal_condition', return_value=[123]), \
             mock.patch(NAMESPACE + '.load_condition_files'):
            key = NetConditionManager()
            key.conditions = {'(+)': {'type': 'normal'}}
            result = key.cal_count_condition(operator_data_list, condition, 'op_name')
        self.assertEqual(result, [])

    def test_cal_normal_condition(self):
        operator_data = [{}]
        condition = {}
        with mock.patch(NAMESPACE + '.MetaConditionManager.cal_normal_condition', return_value=[1]), \
             mock.patch(NAMESPACE + '.load_condition_files'):
            key = NetConditionManager()
            result = key.cal_normal_condition(operator_data, condition, 'op_name')
        self.assertEqual(result, [1])

    def test_cal_formula_condition(self):
        operator_data = [{}]
        condition = {}
        with mock.patch(NAMESPACE + '.MetaConditionManager.cal_formula_condition', return_value=[2]), \
             mock.patch(NAMESPACE + '.load_condition_files'):
            key = NetConditionManager()
            result = key.cal_formula_condition(operator_data, condition, 'op_name')
        self.assertEqual(result, [2])

    def test_cal_accumulate_condition(self):
        op1 = {'op_name': 'op1', 'task_wait_time': 1, 'task_duration': 2}
        op2 = {'op_name': 'op2', 'task_wait_time': 1, 'task_duration': 2}
        op3 = {'op_name': 'op3', 'task_wait_time': 1, 'task_duration': 2}
        op4 = {'op_name': 'op4', 'task_wait_time': 1, 'task_duration': 2}
        operator_data_list = [op1, op2, op3, op4]
        condition = {'dependency': '', 'threshold': '0.1', 'cmp': '>', 'accumulate': ['task_wait_time'],
                     'compare': ['task_duration', 'task_wait_time']}
        with mock.patch(NAMESPACE + '.load_condition_files'):
            key = NetConditionManager()
            key.conditions = {'(+)': {'type': 'normal'}}
            result = key.cal_accumulate_condition(operator_data_list, condition, 'op_name')
        self.assertEqual(len(result), 4)


def test_get_two_decimal():
    num = 1
    result = get_two_decimal(num)
    unittest.TestCase().assertEqual(result, 1)


if __name__ == '__main__':
    unittest.main()
