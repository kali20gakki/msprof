from collections import deque
from profiling_sql_framework.prof_temp_bean import ProfBean


class JoinOperation:
    def __init__(self: any, left_data: list, right_data: list, join_attr_list: list, concat_left_queue: any) -> None:
        self.left_data = left_data
        self.right_data = right_data
        self.join_attr_list = join_attr_list
        self.concat_left_queue = concat_left_queue
        self.get_groupby_join_attr()

    def get_groupby_join_attr(self: any) -> None:
        self.groupby_join_attr = {}
        for left_datum in self.left_data:
            join_value = tuple(getattr(left_datum, attr) for attr, _ in self.join_attr_list)
            left_data = self.groupby_join_attr.setdefault(join_value, deque([]))
            left_data.append(left_datum)

    def join(self: any) -> any:
        for right_datum in self.right_data:
            join_value = tuple(getattr(right_datum, attr) for _, attr in self.join_attr_list)
            left_data = self.groupby_join_attr.get(join_value)

            for left_datum in self.concat_left_queue(left_data):
                yield self.concat(left_datum, right_datum)

    def concat(self: any, left_datum: any, right_datum: any) -> any:
        prof_bean = ProfBean()
        for attr, value in left_datum.__dict__.items():
            setattr(prof_bean, attr, value)

        for attr, value in right_datum.__dict__.items():
            setattr(prof_bean, attr, value)

        return prof_bean
