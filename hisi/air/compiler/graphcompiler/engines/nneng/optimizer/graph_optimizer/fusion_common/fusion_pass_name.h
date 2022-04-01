/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_FUSION_COMMON_FUSION_PASS_NAME_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_FUSION_COMMON_FUSION_PASS_NAME_H_
#include <map>
#include <string>

namespace fe {
// graph fusion pass name
const std::string BATCHNORM_FUSION_PASS = "BatchnormFusionPass";
const std::string CONV_BATCHNORM_FUSION_PASS = "ConvBatchnormFusionPass";
const std::string CONV_SCALE_FUSION_PASS = "ConvScaleFusionPass";
const std::string RELU_FUSION_PASS = "ReluFusionPass";
const std::string EXTREMUM_GRAD_FUSION_PASS = "ExtremumGradFusionPass";
const std::string LOGSOFTMAX_GRAD_FUSION_PASS = "LogSoftmaxGradFusionPass";
const std::string MATMUL_CAST_FUSION_PASS = "MatmulCastFusionPass";
const std::string MATMUL_BIASADD_FUSION_PASS = "MatMulBiasAddFusionPass";
const std::string DRELU_FUSION_PASS = "DreluFusionPass";
const std::string BATCHNORM_BNINFER_FUSION_PASS = "BatchNormBnInferFusionPass";
const std::string PSROIPOOLING_FUSION_PASS = "PSROIPoolingFusionPass";
const std::string HostBN_Fusion_Pass = "HostBNFusionPass";
const std::string CONV_WEIGHT_COMPRESS_FUSION_PASS = "ConvWeightCompressFusionPass";
const std::string SPLIT_CONV_CONCAT_FUSION_PASS = "SplitConvConcatFusionPass";
const std::string CONV_CONCAT_FUSION_PASS = "ConvConcatFusionPass";
const std::string SPLIT_CONV_FUSION_PASS = "SplitConvFusionPass";
const std::string CONCAT_QUANT_FUSION_PASS = "ConcatQuantFusionPass";

const std::string STRIDE_HOISTING_PASS = "StrideHoistingPass";

// quant graph fusion pass name
const std::string AVGPOOL_QUANT_ROLLBACK_BIAS_PASS = "AvgPoolQuantProcessFusionPass";
const std::string CONV2D_QUANT_ROLLBACK_BIAS_PASS = "Conv2DQuantProcessFusionPass";
const std::string GROUP_CONV2D_QUANT_ROLLBACK_BIAS_PASS = "GroupConv2DQuantProcessFusionPass";
const std::string CONV2DTD_QUANT_ROLLBACK_BIAS_PASS = "Conv2DTDQuantProcessFusionPass";
const std::string DECONV_QUANT_ROLLBACK_BIAS_PASS = "DeConvQuantProcessFusionPass";
const std::string DWCONV2D_QUANT_ROLLBACK_BIAS_PASS = "DWConv2DQuantProcessFusionPass";
const std::string FC_QUANT_ROLLBACK_BIAS_PASS = "FCQuantProcessFusionPass";
const std::string MATMULV2_QUANT_ROLLBACK_BIAS_PASS = "MatmulV2QuantProcessFusionPass";
const std::string POOLING_QUANT_ROLLBACK_BIAS_PASS = "PoolingQuantProcessFusionPass";
const std::string MAXPOOL_QUANT_ROLLBACK_BIAS_PASS = "MaxPoolQuantProcessFusionPass";
const std::string BATCH_MATMULV2_QUANT_ROLLBACK_BIAS_PASS = "BatchMatmulV2QuantProcessFusionPass";

const std::string V100_REQUANT_FUSION_PASS = "V100RequantFusionPass";
const std::string V100_NOT_REQUANT_FUSION_PASS = "V100NotRequantFusionPass";
const std::string V200_REQUANT_FUSION_PASS = "V200RequantFusionPass";
const std::string V200_NOT_REQUANT_FUSION_PASS = "V200NotRequantFusionPass";
const std::string TF_TAG_NO_CONST_FOLDING_FUSION_PASS = "TfTagNoConstFoldingFusionPass";
const std::string TF_MERGE_CONV2DBACKPROPINPUT_FUSION_PASS = "TfMergeConv2DBackpropInputFusionPass";
const std::string TF_MERGE_WEIGHT_QUANT_FUSION_PASS = "TfMergeWeightQuantFusionPass";
const std::string DELETE_NO_CONST_FOLDING_FUSION_PASS = "DeleteNoConstFoldingFusionPass";
// ub fusion pass
const std::string CONV2D_BACKPROP_ELEMWISE_UB_PASS = "TbeConv2DBackpropElemwiseFusionPass";
const std::string CONV_BNREDUCE_UB_PASS = "TbeConvBnreduceFusionPass";
const std::string CONV_DEQUANT_QUANT_UB_PASS = "TbeConvDequantQuantFusionPass";
const std::string CONV_DEQUANT_UB_PASS = "TbeConvDequantFusionPass";
const std::string CONV_REQUANT_UB_PASS = "TbeConvRequantFusionPass";
const std::string BNUPDATE_ELTWISE_UB_PASS = "TbeBnupdateEltwiseFusionPass";
const std::string BNUPDATE_ELTWISE_ELTWISE_UB_PASS = "TbeBnupdateEltwiseEltwiseFusionPass";
const std::string MULTIOUTPUT_UB_PASS = "TbeMultiOutputFusionPass";
const std::string REDUCE_ELEMWISE_UB_PASS = "TbeReduceElemwiseFusionPass";
const std::string ELTWISE_UB_PASS = "TbeEltwiseFusionPass";
const std::string DYNAMIC_ELEMWISE_BROADCAST_UB_PASS = "TbeDynamicElemwiseBroadcastFusionPass";
const std::string DYNAMIC_ELEMWISE_REDUCE_UB_PASS = "TbeDynamicElemwiseReduceFusionPass";
const std::string DEPTHWISECONV_ELEMWISE_UB_PASS = "TbeDepthwiseConvElemwiseFusionPass";
const std::string DEPTHWISECONV_DEQUANT_UB_PASS = "TbeDepthwiseConvDequantFusionPass";
const std::string READSELECT_ELTWISE_UB_PASS = "TbeReadSelectEltwiseFusionPass";
const std::string ELTWISE_WRITESELECT_UB_PASS = "TbeEltwiseWriteSelectFusionPass";
const std::string ELTWISE_QUANT_UB_PASS = "TbeEltwiseQuantFusionPass";
const std::string COMMON_RULES0_UB_PASS = "TbeCommonRules0FusionPass";
const std::string COMMON_RULES2_UB_PASS = "TbeCommonRules2FusionPass";
const std::string ELEMWISE_QUANT_UB_PASS = "TbeElemwiseQuantFusionPass";
const std::string FIXPIPE_FUSION_PASS = "TbeFixPipeFusionPass";
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_FUSION_COMMON_FUSION_PASS_NAME_H_
