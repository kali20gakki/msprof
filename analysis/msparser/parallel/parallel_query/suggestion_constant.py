from common_func.db_name_constant import DBNameConstant


class SuggestionConstant:
    SUGGESTIONS = {
        "data-parallel": {
            1: "Gradient segmentation is not performed on the model. You can apply a proper gradient segmentation "
               "policy to improve the parallelism degree of computation and AllReduce operators, "
               "thereby shortening the step tail time. For details, "
               "see the all_reduce_fusion_config parameter settings in the MindSpore distributed parallelism tutorial.",
            2: "Gradient segmentation of the model has not achieved the optimal effect. In ideal situations{}. "
               "You can adjust the gradient segmentation policy to improve the parallelism degree of computation "
               "and AllReduce operators. For details, see the all_reduce_fusion_config parameter "
               "settings in the MindSpore distributed parallelism tutorial.",
            3: "The model has achieved an ideal gradient segmentation effect."},
        "model-parallel": {
            1: "Operator tiling of the model has not achieved the optimal effect. In ideal situations, the "
               "proportion of the pure communication time should be less than 10% (current value: {}). "
               "You can enable the AllToAll communication operator and use a different policy search algorithm. "
               "For details, see the enable_alltoall and search_mode parameter settings in the MindSpore "
               "distributed parallelism tutorial. If the effect is still unsatisfactory, you can manually tile "
               "the operators to improve the parallelism degree of computation and communication operators.",
            2: "Operator tiling of the model has not achieved the optimal effect. In ideal situations, the "
               "proportion of the pure communication time should be less than 10% (current value: {}). "
               "You can adjust the operator tiling policy and enable the AllToAll communication operator to "
               "improve the parallelism degree of computation and communication operators. For details, "
               "see the enable_alltoall parameter settings in the MindSpore distributed parallelism tutorial.",
            3: "The model has achieved an ideal operator tiling effect."},
        "pipeline-parallel": {
            1: "Operator tiling of the model has not achieved the optimal effect. In ideal situations, "
               "the proportion of the pure communication time (receive op not contained) should be less than "
               "10% (current value: {}). You can adjust the operator tiling policy and enable the AllToAll "
               "communication operator to improve the parallelism degree of computation and communication "
               "operators. For details, see the enable_alltoall parameter settings in the "
               "MindSpore distributed parallelism tutorial.",
            2: "Stage division of the model has not achieved the optimal effect. In ideal situations{}. "
               "You can adjust the stage division policy to balance the computing workload among stages and "
               "improve the parallelism degree of computation and communication operators. For details, see the "
               "pipeline_stages, pipeline_stage, and micro_size parameter settings in the "
               "MindSpore distributed parallelism tutorial.",
            3: "The model has achieved ideal operator tiling and stage division effects."}
    }
