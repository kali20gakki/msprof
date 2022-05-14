import unittest
from mscalculate.step_trace.create_step_table import CreateTrainingTrace

def get_data():

    return {
            1: {
                1: {
                    "step_start": 2,
                    "step_end": 7,
                    "training_trace": {"fp": 3, "bp": 5},
                    "all_reduce": [{"reduce_start": 4, "reduce_end": 6}]
                },

                2: {
                    "step_start": 7,
                    "step_end": 12,
                    "training_trace": {"fp": 9, "bp": 11},
                    "all_reduce": [{"reduce_start": 8, "reduce_end": 10}]
                },

                3: {
                    "step_start": 16,
                    "step_end": 18,
                    "training_trace": {},
                    "all_reduce": []

                },

                4: {
                    "step_start": 19,
                    "step_end": 21,
                    "training_trace": {"fp": 20},
                    "all_reduce": []
                },

                5: {
                    "step_start": 22,
                    "step_end": 24,
                    "training_trace": {},
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
        CreateTrainingTrace.run(sample_config, A)
        self.assertEqual(CreateTrainingTrace.data,
                         [[None, 1, 1, 3, 5, 7, 5, 2, 2, 0],
                          [None, 1, 2, 9, 11, 12, 5, 2, 1, 2],
                          [None, 1, 3, 0, 0, 18, 2, 0, 0, 0],
                          [None, 1, 4, 20, 0, 21, 2, 0, 0, 2],
                          [None, 1, 5, 0, 0, 24, 2, 0, 0, 0]]
                         )


if __name__ == '__main__':
    unittest.main()