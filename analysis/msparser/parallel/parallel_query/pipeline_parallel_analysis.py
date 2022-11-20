from msmodel.parallel.cluster_parallel_model import ClusterParallelViewModel
from msparser.parallel.parallel_query.suggestion_constant import SuggestionConstant


class PipelineParallelAnalysis:
    def __init__(self: any, params: dict):
        self._params = params

    def get_parallel_data(self: any) -> dict:
        with ClusterParallelViewModel(self._params["collection_path"]) as _model:
            first_field_name, first_header_name = _model.get_first_field_name(self._params)
            condition = _model.get_parallel_condition(self._params)
            parallel_data = _model.get_pipeline_parallel_data(first_field_name, condition)
        return {"parallel_mode": "Pipeline Parallel",
                "headers": [first_header_name, "Computation Time", "Pure Communication Time(Only Receice Op Contained)",
                            'Pure Communication Time(Receice Op Not Contained)', 'Stage Time'],
                "data": parallel_data}

    def get_tuning_suggestion(self: any) -> dict:
        with ClusterParallelViewModel(self._params["collection_path"]) as _model:
            tuning_data = _model.get_pipeline_parallel_tuning_data()
        suggestion = {"parallel_mode": "Pipeline Parallel",
                      "suggestion": []}
        if not tuning_data:
            return suggestion
        if tuning_data[0][0] > 0.1:
            suggestion.get("suggestion").append(
                SuggestionConstant.SUGGESTIONS.get("pipeline-parallel").get(1).format(
                    '{:.1%}'.format(tuning_data[0][0])))
        index_desc = ""
        if tuning_data[0][1] > 0.1:
            index_desc = index_desc + ", the proportion of the pure communication time (only receive op " \
                                      "contained) should be less than 10% (current value: {})".format(
                '{:.1%}'.format(tuning_data[0][1]))
        if tuning_data[0][2] > 0:
            index_desc = index_desc + ", the deviation between the stage time of all devices and the " \
                                      "average stage time should not exceed 20%"
        if index_desc:
            suggestion.get("suggestion").append(
                SuggestionConstant.SUGGESTIONS.get("pipeline-parallel").get(2).format(index_desc))
        if not suggestion.get("suggestion"):
            suggestion.get("suggestion").append(SuggestionConstant.SUGGESTIONS.get("pipeline-parallel").get(3))
        return suggestion
