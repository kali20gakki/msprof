import unittest
from mscalculate.step_trace.create_step_table import CreateStepTraceData

def get_data():

    return {
            1: {
                1: {
                    "step_start": 2,
                    "step_end": 7,
                    "training_trace": [{"fp": 3, "bp": 5}],
                    "all_reduce": [{"reduce_start": 4, "reduce_end": 6}]
                },

                2: {
                    "step_start": 7,
                    "step_end": 12,
                    "training_trace": [{"fp": 9, "bp": 11}],
                    "all_reduce": [{"reduce_start": 8, "reduce_end": 10}]
                },

                3: {
                    "step_start": 16,
                    "step_end": 18,
                    "training_trace": [],
                    "all_reduce": []

                },

                4: {
                    "step_start": 19,
                    "step_end": 21,
                    "training_trace": [{"fp": 20, "bp": None}],
                    "all_reduce": []
                },

                5: {
                    "step_start": 22,
                    "step_end": 24,
                    "training_trace": [],
                    "all_reduce": [{'reduce_end': None, 'reduce_start': 23}]
                }

            }
        }


class A:

    @classmethod
    def get_data(cls):
        collect_data = get_data()
        return collect_data


class TestCreateStepTraceData(unittest.TestCase):
    def test_run(self):
        sample_config = {"result_dir": "./"}
        CreateStepTraceData.run(sample_config, A)
        self.assertEqual(CreateStepTraceData.data,
                         [[1, 1, 2, 7, 1],
                          [2, 1, 7, 12, 2],
                          [3, 1, 16, 18, 3],
                          [4, 1, 19, 21, 4],
                          [5, 1, 22, 24, 5]])


if __name__ == '__main__':
    unittest.main()