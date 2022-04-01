#ifndef _KERNEL_PARAMS_
#define _KERNEL_PARAMS_

#include "dnn_base.h"
#include "cce/cce_def.hpp"

namespace cce
{
namespace kernel
{
#define QUANTIZE_MAGIC              0xDEADDEAD

typedef enum tagKernelDataType
{
  KERNEL_DATA_TYPE_FP16 = 0,
  KERNEL_DATA_TYPE_INT8,
  KERNEL_DATA_TYPE_UINT8,
  KERNEL_DATA_TYPE_DUAL_SUB_UINT8,
  KERNEL_DATA_TYPE_DUAL_SUB_INT8,
  KERNEL_DATA_TYPE_INT32,
  KERNEL_DATA_TYPE_DUAL,
  KERNEL_DATA_TYPE_INT64,
  KERNEL_DATA_TYPE_FLOAT,
  KERNEL_DATA_TYPE_DOUBLE,
  KERNEL_DATA_TYPE_BOOL,
  KERNEL_DATA_TYPE_INT16,
  KERNEL_DATA_TYPE_UINT16,
  KERNEL_DATA_TYPE_UINT32,
  KERNEL_DATA_TYPE_UINT64,
  KERNEL_DATA_TYPE_FP32,
  KERNEL_DATA_TYPE_RESERVED,
} kDataType_t;

typedef enum tagKernelTensorFormat
{
  KERNEL_TENSOR_FORMAT_NCHW = 0,
  KERNEL_TENSOR_FORMAT_NHWC,
  KERNEL_TENSOR_FORMAT_NCHW_VEC16,
  KERNEL_TENSOR_FORMAT_NCHW_VEC8,
  KERNEL_TENSOR_FORMAT_NCHW_RGB,
  KERNEL_TENSOR_FORMAT_NC1HWC0,
  KERNEL_TENSOR_FORMAT_C1HWNC0,
  KERNEL_TENSOR_FORMAT_FRACTAL_Z,
  KERNEL_TENSOR_FORMAT_ND,
  KERNEL_TENSOR_FORMAT_NC1C0HWPAD,
  KERNEL_TENSOR_FORMAT_NHWC1C0,
  KERNEL_TENSOR_FORMAT_FSR_NCHW,
  KERNEL_TENSOR_FORMAT_FRACTAL_DECONV,
  KERNEL_TENSOR_FORMAT_FRACTAL_DECONV_SP_STRIDE,
  KERNEL_TENSOR_FORMAT_FRACTAL_DECONV_SP_STRIDE8,
  KERNEL_TENSOR_FORMAT_RESERVED,
  KERNEL_TENSOR_FORMAT_NC1HWC0_C04,
  KERNEL_TENSOR_FORMAT_HWCN,
  KERNEL_TENSOR_FORMAT_FRACTAL_Z_C04
} kTensorFormat_t;

typedef enum tagKernelQuantizeType
{
  KERNEL_QUANTIZE_TYPE_ANDROIDNN = 0,
} kQuantizeType_t;

typedef struct tagKernelTensor
{
  kTensorFormat_t format;
  kDataType_t dataType;
  uint32_t batchSize;
  uint32_t channel;
  uint32_t height;
  uint32_t width;
  uint32_t channel0;
  uint32_t padC0;
  uint32_t dataSize;
  int32_t dimCnt;
  int32_t dim[CC_DIM_MAX];
  int32_t realDimCnt;
  ccVecQuantizePara_t vecQuantizePara;
} kTensor_t;

typedef struct tagKernelFilter
{
  kTensorFormat_t format;
  kDataType_t dataType;
  uint32_t output;
  uint32_t channel;
  uint32_t height;
  uint32_t width;
  uint32_t dataSize;
} kFilter_t;

typedef struct tagKernelConvParam
{
  cce::ccConvolutionMode_t mode;
  cce::ccConvolutionFwdAlgo_t algo;
  uint32_t padHHead;
  uint32_t padHTail;
  uint32_t padWHead;
  uint32_t padWTail;
  uint32_t verticalStride;
  uint32_t horizontalStride;
  uint32_t dilationHeight;
  uint32_t dilationWidth;
  cce::ccQuantize_t quantizeParaInfo;
  cce::ccConvolutionAipp_t aippInfo;
  uint32_t adjH;
  uint32_t adjW;
  uint32_t targetShapeH;
  uint32_t targetShapeW;
  uint32_t reluFlag;
  uint32_t deconvSpStrideMark;
  uint32_t group;
  uint64_t concatBatchSize;
} kConvParam_t;

typedef struct tagKernelConvBwdParam
{
  cce::ccConvolutionMode_t mode;
  cce::ccConvolutionBwdAlgo_t algo;
  uint32_t padHHead;
  uint32_t padHTail;
  uint32_t padWHead;
  uint32_t padWTail;
  uint32_t verticalStride;
  uint32_t horizontalStride;
  uint32_t dilationHeight;
  uint32_t dilationWidth;
  cce::ccQuantize_t quantizeParaInfo;
  cce::ccConvolutionAipp_t aippInfo;
  uint32_t adjH;
  uint32_t adjW;
  uint32_t targetShapeH;
  uint32_t targetShapeW;
  uint32_t reluFlag;
  uint32_t deconvSpStrideMark;
} kConvBwdParam_t;

typedef struct tagKernelPoolingParam
{
  cce::ccPoolingMode_t poolingMode;
  uint32_t windowHeight;
  uint32_t windowWidth;
  uint32_t poolPadTop;
  uint32_t poolPadBottom;
  uint32_t poolPadLeft;
  uint32_t poolPadRight;
  uint32_t verticalStride;
  uint32_t horizontalStride;
  uint32_t poolDataMode;
  uint32_t poolCeilMode;
  uint64_t factorAddr;
  cce::ccQuantize_t quantizeParaInfo;
  cce::ccPooingFwdAlgo_t algo;
} kPoolingParam_t;

typedef struct tagKernelSoftmaxParam
{
  ccSoftmaxAlgo_t algo;
  int32_t softmaxAxis;
  uint32_t padNum;
  float beta;
  int32_t hAndWSwitchEn;
} kSoftmaxParam_t;

typedef struct tagKernelLrnParam
{
  ccLRNMode_t lrnMode;
  int32_t lrnN;
  double lrnAlpha;
  double lrnBeta;
  double lrnK;
} kLrnParam_t;

typedef struct tagKernelFcUnzipParam
{
  bool isUnzip;
} kFcUnzipParam_t;

}  // namespace kernel
}  // namespace cce

#endif