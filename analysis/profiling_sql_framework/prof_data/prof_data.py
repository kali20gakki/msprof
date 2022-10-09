from profiling_sql_framework.prof_temp_bean import ProfBean
from profiling_sql_framework.prof_data.join_operation import JoinOperation


class ProfData:
    def __init__(self: any, data: any) -> None:
        self.data = data

    def select(self: any) -> any:
        self.data = list(self.data)
        return self

    def choose(self: any, *choosed_attr) -> any:
        def choose_generator():
            for datum in self.data:
                prof_bean = ProfBean()
                for attr in choosed_attr:
                    setattr(prof_bean, attr, getattr(datum, attr))
                    yield prof_bean
        return ProfData(choose_generator())

    def where(self: any, condition_func: any) -> any:
        data_generator = filter(condition_func, self.data)
        return ProfData(data_generator)

    def mapping(self: any, condition_func: any, new_attr: str) -> any:
        def mapping_data_generator():
            for datum in self.data:
                setattr(datum, new_attr, condition_func(datum))
                yield datum
        return ProfData(mapping_data_generator())

    def inner_join(self: any, prof_data: any, join_attr_list: list) -> any:
        def concat_left_queue(left_data):
            return left_data
        return ProfData(JoinOperation(self.data, prof_data.data, join_attr_list, concat_left_queue))

    def except_join(self: any, prof_data: any, join_attr_list: list) -> any:
        def concat_left_queue(left_data):
            datum = left_data.popleft()
            return [datum]
        return ProfData(JoinOperation(self.data, prof_data.data, join_attr_list, concat_left_queue))
