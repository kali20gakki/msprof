from msmodel.parallel.cluster_parallel_model import ClusterParallelViewModel
from msparser.parallel.parallel_query.suggestion_constant import SuggestionConstant


class DataParallelAnalysis:
    def __init__(self: any, params: dict):
        self._params = params

    def get_parallel_data(self: any) -> dict:
        with ClusterParallelViewModel(self._params["collection_path"]) as _model:
            first_field_name, first_header_name = _model.get_first_field_name(self._params)
            condition = _model.get_parallel_condition(self._params)
            parallel_data = _model.get_data_parallel_data(first_field_name, condition)
        return {"parallel_mode": "Data Parallel",
                "headers": [first_header_name, "Computation Time", "Pure Communication Time",
                            "Communication Time", "Communication Interval"],
                "data": parallel_data}

    def get_tuning_suggestion(self: any) -> dict:
        with ClusterParallelViewModel(self._params["collection_path"]) as _model:
            tuning_data = _model.get_data_parallel_tuning_data()
        suggestion = {"parallel_mode": "Data Parallel",
                      "suggestion": []}
        if not tuning_data:
            return suggestion
        if tuning_data[0][0] == 1:
            suggestion.get("suggestion").append(SuggestionConstant.SUGGESTIONS.get("data-parallel").get(1))
        elif tuning_data[0][1] <= 0.4 and tuning_data[0][2] <= 0.3:
            suggestion.get("suggestion").append(SuggestionConstant.SUGGESTIONS.get("data-parallel").get(3))
        else:
            index_desc = ""
            if tuning_data[0][1] > 0.4:
                index_desc = index_desc + ", the proportion of the pure communication time of the AllReduce " \
                                          "operator to the total communication time should be less than " \
                                          "40% (current value: {})".format('{:.1%}'.format(tuning_data[0][1]))
            if tuning_data[0][2] > 0.3:
                index_desc = index_desc + ", the proportion of the AllReduce operator interval to the total " \
                                          "communication time should be less than 30% (current value: {})".format(
                    '{:.1%}'.format(tuning_data[0][2]))
            suggestion.get("suggestion").append(
                SuggestionConstant.SUGGESTIONS.get("data-parallel").get(2).format(index_desc))
        return suggestion
