/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
 * Description: fusion_api.h
 */

#include "tensor_engine/fusion_api.h"

namespace te {

/**
 * @brief: Tbe Initialize proccess
 * @param [in] options            ddk version
 * @return [out] bool             succ or fail for Tbe Initialize
 */
bool TbeInitialize(const std::map<std::string, std::string> &options, bool *isSupportParallelCompilation)
{
  *isSupportParallelCompilation = true;
  return true;
}

/**
 * @brief: tbe finalize proccess
 * @return [out] bool             succ or fail for Tbe Finalize
 */
bool TbeFinalize()
{
  return true;
}

/**
 * @brief: get finished compilation task list
 * @return [out] list             finished compilation task list
 */
bool WaitAllFinished(uint64_t graphId, vector<FinComTask> &tasks)
{
  return true;
}

/*
 * @brief: check tbe op capability
 * @param [in] opinfo: op total parameter set
 * @param [out] IsSupport: result to support or not
 * @return bool: check process ok or not
 */
bool CheckTbeSupported(TbeOpInfo& opinfo, CheckSupportedResult &isSupport, std::string &reason)
{
  return true;
}

/**
 * @brief pre build tbe op
 * @return [out] bool                 succ or fail of prebuild
 */
bool PreBuildTbeOp(TbeOpInfo& opinfo, uint64_t taskId, uint64_t graphId)
{
  return true;
}

/**
 * @brief build fusion op
 * @param [in] teGraphNode            graph node ptr vector
 * @param [in] strategy               op optimization strategy, empty or json str with key
 *                                    module_name, object_name and object_value
 * @return [in] outputNode            recode json file path in it
 * @return [out] bool                 succ or fail of building fusion op
 */
OpBuildResCode TeFusion(std::vector<ge::Node *> teGraphNode, ge::OpDescPtr opDesc,
                        const std::vector<ge::NodePtr> &toBeDel,
                        uint64_t taskId, uint64_t graphId, const std::string &strategy)
{
  return OP_BUILD_SUCC;
}

OpBuildResCode TeFusionV(std::vector<ge::Node *> teGraphNode, ge::OpDescPtr opDesc,
                         const std::vector<ge::NodePtr> &toBeDel, uint64_t taskId, uint64_t graphId,
                         uint64_t sgtThreadIndex, const std::string &strategy)
{
  return OP_BUILD_SUCC;
}

/**
 * @brief select tbe op format
 * @param [in] tbeOpInfo            op info
 * @param [out] opDtypeformat       op date format
 * @return [out] bool               succ or fail
 */
bool SelectTbeOpFormat (const TbeOpInfo &tbeOpInfo, std::string &opDtypeFormat)
{
  return true;
}

/**
 * @brief clear instance
 * @return [out] bool                 succ or fail of TeFusionEnd
 */
bool TeFusionEnd()
{
  return true;
}

/**
 * @brief get op info
 * @param [in] tbeOpInfo            op info
 * @param [out] result              op segmentation information
 * @return [out] LX_QUERY_STATUS    LX_QUERY_FAIL, LX_QUERY_NOT_FOUND or LX_QUERY_SUCC
 */
LX_QUERY_STATUS GetOpInfo(const TbeOpInfo &tbeOpInfo, std::string &result)
{
  return LX_QUERY_SUCC;
}

/**
 * @brief fuzz build tbe op
 * @param [in] tbeOpInfo            op info
 * @param [in] taskId               fuzz build task id
 * @param [in] graphId              fuzz build graph id
 * @param [in] node                 tbe op to compile
 * @return [out] OpBuildResCode                 succ or fail of building fusion op
 */
OpBuildResCode FuzzBuildTbeOp(uint64_t taskId, uint64_t graphId, ge::Node &node)
{
  return OP_BUILD_SUCC;
}

/**
 * @brief set fuzz build support info to node
 * @param [in] nodePtr              original node
 * @param [in] supportInfoStr       generate support info
 * @param [in] indirectIndexs       indexs of current node indirect link to original node
 * @param [in] directIndexMap       index map of current node input index direct link to original node
 * @return [out] bool               succ or fail of set fuzz build support info to node
 */
bool SetFuzzyBuildSupportInfoToNode(ge::NodePtr &nodePtr, const std::string &supportInfoStr,
                                    const std::vector<size_t> &indirectIndexs,
                                    const std::vector<std::pair<size_t, size_t>> &directIndexMap)
{
  return true;
}

/**
 * @brief check is this op has registered tbegeneralize func
 * @param [in] tbeOpInfo            op info
 * @param [in] hasRegisteredFunc    true or false
 * @return [out] bool               succ or fail of check
 */
bool CheckIsTbeGeneralizeFuncRegistered(const TbeOpInfo &tbeOpInfo, bool &hasRegisteredFunc)
{
  return true;
}

/**
 * @brief shape or value generalize with generalize type
 * @param [in] tbeOpInfo            op info
 * @param [in] generalize_type      generalize type
 * @param [in] nodePtr              node
 * @return [out] bool               succ or fail of check
 */
bool TeGeneralize(const TbeOpInfo &tbeOpInfo, const TE_GENERALIZE_TYPE &generalize_type, const ge::NodePtr &nodePtr)
{
  return true;
}

/**
 * @brief get op specific info from tbe
 * @param [in] tbeOpInfo         op info
 * @param [in] opSpecificInfo    op specific info
 * @param [out] bool             succ or fail
 */
bool GetOpSpecificInfo(const TbeOpInfo &tbeOpInfo, std::string &opSpecificInfo)
{
  return true;
}

/**
 * @brief Check the validity of shape range by tbe
 * @param [in] tbeOpInfo                  op info
 * @param [out] isSupported               shape range is valid or not
 * @param [out] unSupportedInputIndexs    shape range unSupported input indexs
 * @param [out] bool                      succ or fail
 */
bool DynamicShapeRangeCheck(const TbeOpInfo &tbeOpInfo, bool &isSupported,
                            std::vector<size_t> &upperLimitedInputIndexs,
                            std::vector<size_t> &lowerLimitedInputIndexs)
{
  return true;
}
} // namespace te

