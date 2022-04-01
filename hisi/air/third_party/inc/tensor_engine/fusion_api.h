/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
 * Description: fusion_api.h
 */

#ifndef TE_FUSION_API_H
#define TE_FUSION_API_H

#include <string>
#include <vector>
#include <tuple>
#include <map>

#include "graph/debug/ge_attr_define.h"
#include "graph/node.h"
#include "runtime/mem.h"


#ifndef RT_MEMORY_L1
#define RT_MEMORY_L1 ((uint32_t)0x1<<16)
#endif

#ifndef RT_MEMORY_L2
#define RT_MEMORY_L2 ((uint32_t)0x1<<17)
#endif

/*lint -e148*/
namespace te {
typedef enum tagTbeMemType {
    TBE_MEMORY_DDR = 0,               /**< DDR memory on device */
    TBE_MEMORY_L1 = 1,               /**< L1 memory on device */
    TBE_MEMORY_L2 = 2,               /**< L2 memory on device */
    TBE_MEMORY_RESERVED,
}tbeMemType_t;

enum ATTR_DTYPE {
    ATTR_INT8             = 0,
    ATTR_UINT8            = 1,
    ATTR_INT16            = 2,
    ATTR_UINT16           = 3,
    ATTR_INT32            = 4,
    ATTR_UINT32           = 5,
    ATTR_INT64            = 6,
    ATTR_UINT64           = 7,
    ATTR_FLOAT32          = 8,
    ATTR_DOUBLE           = 9,
    ATTR_BOOL             = 10,
    ATTR_STR              = 11,
    ATTR_LIST_INT8        = 12,
    ATTR_LIST_UINT8       = 13,
    ATTR_LIST_INT16       = 14,
    ATTR_LIST_UINT16      = 15,
    ATTR_LIST_INT32       = 16,
    ATTR_LIST_UINT32      = 17,
    ATTR_LIST_INT64       = 18,
    ATTR_LIST_UINT64      = 19,
    ATTR_LIST_FLOAT32     = 20,
    ATTR_LIST_DOUBLE      = 21,
    ATTR_LIST_BOOL        = 22,
    ATTR_LIST_STR         = 23,
    ATTR_LIST_LIST_INT64  = 24,
    ATTR_LIST_LIST_FLOAT  = 25,

    // illegal type which can't be fused
    ATTR_MAX,
};

static std::map<ATTR_DTYPE, std::string> TbeAttrDtypeToStringMap = {
    {ATTR_INT8,            "int8"},
    {ATTR_UINT8,           "uint8"},
    {ATTR_INT16,           "int16"},
    {ATTR_UINT16,          "uint16"},
    {ATTR_INT32,           "int32"},
    {ATTR_UINT32,          "uint32"},
    {ATTR_INT64,           "int64"},
    {ATTR_UINT64,          "uint64"},
    {ATTR_FLOAT32,         "float32"},
    {ATTR_DOUBLE,          "double"},
    {ATTR_BOOL,            "bool"},
    {ATTR_STR,             "str"},
    {ATTR_LIST_INT8,       "list_int8"},
    {ATTR_LIST_UINT8,      "list_uint8"},
    {ATTR_LIST_INT16,      "list_int16"},
    {ATTR_LIST_UINT16,     "list_uint16"},
    {ATTR_LIST_INT32,      "list_int32"},
    {ATTR_LIST_UINT32,     "list_uint32"},
    {ATTR_LIST_INT64,      "list_int64"},
    {ATTR_LIST_UINT64,     "list_uint64"},
    {ATTR_LIST_FLOAT32,    "list_float32"},
    {ATTR_LIST_DOUBLE,     "list_double"},
    {ATTR_LIST_BOOL,       "list_bool"},
    {ATTR_LIST_STR,        "list_str"},
    {ATTR_LIST_LIST_INT64, "list_list_int64"},
    {ATTR_LIST_LIST_FLOAT, "list_list_float"},
};

enum ATTR_SHAPETYPE {
    ATTR_SHAPE_LIST     = 0,
    ATTR_SHAPE_TUPLE    = 1,
    ATTR_SHAPE_TYPE_MAX = ATTR_SHAPE_TUPLE,
};

enum LX_QUERY_STATUS {
    LX_QUERY_FAIL       = -1,
    LX_QUERY_NOT_FOUND  = 0,
    LX_QUERY_SUCC       = 1
};

enum CheckSupportedResult {
  NOT_SUPPORTED = 0,
  FULLY_SUPPORTED = 1,
  PARTIALLY_SUPPORTED = 2
};

enum OP_VALUE_DEPEND {
    VALUE_DEPEND_IGNORE = 0,
    VALUE_DEPEND_REQUIRED,
    VALUE_DEPEND_OPTIONAL
};

enum TE_GENERALIZE_TYPE {
    REGISTER_FUNC = 0,
    DEFAULT_TBE_OP_INFO,
    DEFAULT_NODE,
    DEFAULT_LIMITED_TBE_OP_INFO,
    GENERALIZE_TYPE_MAX
};

const std::string NO_NEED_REASON = "No_Need_Reason";
const std::string CORE_TYPE_NAME = "coreType";

const std::string RANGE_LIMITED = "limited";
const std::string RANGE_UNLIMITED = "unlimited";
const std::string RANGE_DYNAMIC = "dynamic";

class TbeOpInfo;

/**
 * @brief: finished compilation task struce
 */
struct FinComTask {
    // indicate which thread exec compilation
    uint64_t graphId;
    // task id
    uint64_t taskId;
    // task status: 0 indicate success, others indicate fail
    int status;
    // error message
    string errMsg;
    // OpDesc with json path writen
    ge::OpDescPtr teNodeOpDesc;
};

class TbeAttrValue {
public:
    TbeAttrValue()
    {
        dtype_ = ATTR_INT8;
    };

    ~TbeAttrValue()
    {
    };

    TbeAttrValue(std::string name, int8_t value):name_(name), int8_value_(value)
    {
        dtype_ = ATTR_INT8;
    };

    TbeAttrValue(std::string name, uint8_t value):name_(name), uint8_value_(value)
    {
        dtype_ = ATTR_UINT8;
    };

    TbeAttrValue(std::string name, int16_t value):name_(name), int16_value_(value)
    {
        dtype_ = ATTR_INT16;
    };

    TbeAttrValue(std::string name, uint16_t value):name_(name), uint16_value_(value)
    {
        dtype_ = ATTR_UINT16;
    };

    TbeAttrValue(std::string name, int32_t value):name_(name), int32_value_(value)
    {
        dtype_ = ATTR_INT32;
    };

    TbeAttrValue(std::string name, uint32_t value):name_(name), uint32_value_(value)
    {
        dtype_ = ATTR_UINT32;
    };

    TbeAttrValue(std::string name, int64_t value):name_(name), int64_value_(value)
    {
        dtype_ = ATTR_INT64;
    };

    TbeAttrValue(std::string name, uint64_t value):name_(name), uint64_value_(value)
    {
        dtype_ = ATTR_UINT64;
    };

    TbeAttrValue(std::string name, float value):name_(name), float_value_(value)
    {
        dtype_ = ATTR_FLOAT32;
    };

    TbeAttrValue(std::string name, double value):name_(name), double_value_(value)
    {
        dtype_ = ATTR_DOUBLE;
    };

    TbeAttrValue(std::string name, bool value):name_(name), bool_value_(value)
    {
        dtype_ = ATTR_BOOL;
    };

    TbeAttrValue(std::string name, std::string value):name_(name), str_value_(value)
    {
        dtype_ = ATTR_STR;
    };

    TbeAttrValue(std::string name, std::vector<int8_t> value):name_(name), list_int8_value_(value)
    {
        dtype_ = ATTR_LIST_INT8;
    };

    TbeAttrValue(std::string name, std::vector<uint8_t> value):name_(name), list_uint8_value_(value)
    {
        dtype_ = ATTR_LIST_UINT8;
    };

    TbeAttrValue(std::string name, std::vector<int16_t> value):name_(name), list_int16_value_(value)
    {
        dtype_ = ATTR_LIST_INT16;
    };

    TbeAttrValue(std::string name, std::vector<uint16_t> value):name_(name), list_uint16_value_(value)
    {
        dtype_ = ATTR_LIST_UINT16;
    };

    TbeAttrValue(std::string name, std::vector<int32_t> value):name_(name), list_int32_value_(value)
    {
        dtype_ = ATTR_LIST_INT32;
    };

    TbeAttrValue(std::string name, std::vector<uint32_t> value):name_(name), list_uint32_value_(value)
    {
        dtype_ = ATTR_LIST_UINT32;
    };

    TbeAttrValue(std::string name, std::vector<int64_t> value):name_(name), list_int64_value_(value)
    {
        dtype_ = ATTR_LIST_INT64;
    };

    TbeAttrValue(std::string name, std::vector<uint64_t> value):name_(name), list_uint64_value_(value)
    {
        dtype_ = ATTR_LIST_UINT64;
    };

    TbeAttrValue(std::string name, std::vector<float> value):name_(name), list_float_value_(value)
    {
        dtype_ = ATTR_LIST_FLOAT32;
    };

    TbeAttrValue(std::string name, std::vector<double> value):name_(name), list_double_value_(value)
    {
        dtype_ = ATTR_LIST_DOUBLE;
    };

    TbeAttrValue(std::string name, std::vector<bool> value):name_(name), list_bool_value_(value)
    {
        dtype_ = ATTR_LIST_BOOL;
    };

    TbeAttrValue(std::string name, std::vector<std::string> value):name_(name), list_str_value_(value)
    {
        dtype_ = ATTR_LIST_STR;
    };

    TbeAttrValue(std::string name, std::vector<std::vector<int64_t>> value):name_(name), list_list_int64_value_(value)
    {
        dtype_ = ATTR_LIST_LIST_INT64;
    };

    TbeAttrValue(std::string name, std::vector<std::vector<float>> value):name_(name), list_list_float_value_(value)
    {
        dtype_ = ATTR_LIST_LIST_FLOAT;
    };

    bool GetType(ATTR_DTYPE& type) const
    {
        type = dtype_;
        return true;
    };

    bool GetName(string& name) const
    {
        name = name_;
        return true;
    };

    bool GetValue(int8_t& value) const
    {
        value = int8_value_;
        return true;
    };

    bool GetValue(uint8_t& value) const
    {
        value = uint8_value_;
        return true;
    };

    bool GetValue(int16_t& value) const
    {
        value = int16_value_;
        return true;
    };

    bool GetValue(uint16_t& value) const
    {
        value = uint16_value_;
        return true;
    };

    bool GetValue(int32_t& value) const
    {
        value = int32_value_;
        return true;
    };

    bool GetValue(uint32_t& value) const
    {
        value = uint32_value_;
        return true;
    };

    bool GetValue(int64_t& value) const
    {
        value = int64_value_;
        return true;
    };

    bool GetValue(uint64_t& value) const
    {
        value = uint64_value_;
        return true;
    };

    bool GetValue(float& value) const
    {
        value = float_value_;
        return true;
    };

    bool GetValue(double& value) const
    {
        value = double_value_;
        return true;
    };

    bool GetValue(bool& value) const
    {
        value = bool_value_;
        return true;
    };

    bool GetValue(std::string& value) const
    {
        value = str_value_;
        return true;
    };

    bool GetValue(std::vector<int8_t>& value) const
    {
        value = list_int8_value_;
        return true;
    };

    bool GetValue(std::vector<uint8_t>& value) const
    {
        value = list_uint8_value_;
        return true;
    };

    bool GetValue(std::vector<int16_t>& value) const
    {
        value = list_int16_value_;
        return true;
    };

    bool GetValue(std::vector<uint16_t>& value) const
    {
        value = list_uint16_value_;
        return true;
    };

    bool GetValue(std::vector<int32_t>& value) const
    {
        value = list_int32_value_;
        return true;
    };

    bool GetValue(std::vector<uint32_t>& value) const
    {
        value = list_uint32_value_;
        return true;
    };

    bool GetValue(std::vector<int64_t>& value) const
    {
        value = list_int64_value_;
        return true;
    };

    bool GetValue(std::vector<uint64_t>& value) const
    {
        value = list_uint64_value_;
        return true;
    };

    bool GetValue(std::vector<float>& value) const
    {
        value = list_float_value_;
        return true;
    };

    bool GetValue(std::vector<double>& value) const
    {
        value = list_double_value_;
        return true;
    };

    bool GetValue(std::vector<bool>& value) const
    {
        value = list_bool_value_;
        return true;
    };

    bool GetValue(std::vector<std::string>& value) const
    {
        value = list_str_value_;
        return true;
    };

    bool GetValue(std::vector<std::vector<int64_t>>& value) const
    {
        value = list_list_int64_value_;
        return true;
    };

    bool GetValue(std::vector<std::vector<float>>& value) const
    {
        value = list_list_float_value_;
        return true;
    };

    bool GetSType(ATTR_SHAPETYPE& stype) const
    {
        stype = stype_;
        return true;
    };

    bool SetSType(ATTR_SHAPETYPE& stype)
    {
        stype_ = stype;
        return true;
    };

    void SetAttrSupAllFlag(const bool &isAttrSupportAllValue)
    {
        isAttrSupportAllValue_ = isAttrSupportAllValue;
    }

    void GetAttrSupAllFlag(bool &isAttrSupportAllValue) const
    {
        isAttrSupportAllValue = isAttrSupportAllValue_;
    }

    void SetAttrHasDefaultValFlag(const bool &isAttrHasDefaultValue)
    {
        isAttrHasDefaultValue_ = isAttrHasDefaultValue;
    }

    void GetAttrHasDefaultValFlag(bool &isAttrHasDefaultValue) const
    {
        isAttrHasDefaultValue = isAttrHasDefaultValue_;
    }

    bool operator==(TbeAttrValue& rObject);

private:
    // need to adapt operator== func while adding new variable
    std::string name_{""};
    int8_t   int8_value_{0};
    uint8_t  uint8_value_{0};
    int16_t  int16_value_{0};
    uint16_t uint16_value_{0};
    int32_t  int32_value_{0};
    uint32_t uint32_value_{0};
    int64_t  int64_value_{0};
    uint64_t uint64_value_{0};
    float float_value_{0.0};
    double double_value_{0.0};
    bool bool_value_{false};
    std::string str_value_{""};

    std::vector<int8_t>   list_int8_value_;
    std::vector<uint8_t>  list_uint8_value_;
    std::vector<int16_t>  list_int16_value_;
    std::vector<uint16_t> list_uint16_value_;
    std::vector<int32_t>  list_int32_value_;
    std::vector<uint32_t> list_uint32_value_;
    std::vector<int64_t>  list_int64_value_;
    std::vector<uint64_t> list_uint64_value_;
    std::vector<float>    list_float_value_;
    std::vector<double>   list_double_value_;
    std::vector<bool>     list_bool_value_;
    std::vector<std::string> list_str_value_;
    std::vector<std::vector<int64_t>> list_list_int64_value_;
    std::vector<std::vector<float>> list_list_float_value_;

    ATTR_DTYPE dtype_{ATTR_FLOAT32};
    ATTR_SHAPETYPE stype_{ATTR_SHAPE_TUPLE};

    // binary infos
    bool isAttrSupportAllValue_ = false;
    bool isAttrHasDefaultValue_ = false;
};


class TbeOpTensor {
public:
    TbeOpTensor()
    {
        sub_format_ = 0;
        stype_ = ATTR_SHAPE_TUPLE;
        is_const_ = false;
        L1_workspace_size_ = -1;
        L1_valid_size_ = -1;
        L1_fusion_type_ = -1;
        addr_offset_ = 0;
        split_index_ = 0;
        addr_type_ = 0;
        use_L1_workspace_ = 0;
        L1_addr_flag_ = -1;
    };

    ~TbeOpTensor()
    {
    };

    TbeOpTensor(std::string name,
                std::vector<int64_t> shape,
                std::string dtype,
                std::string format,
                ATTR_SHAPETYPE stype = ATTR_SHAPE_TUPLE)
        : name_(name),
          shape_(shape),
          shapeRange_(std::tuple<bool, std::vector<std::pair<int64_t, int64_t>>>{false, {}}),
          format_(format),
          sub_format_(0),
          originShapeRange_(std::tuple<bool, std::vector<std::pair<int64_t, int64_t>>>{false, {}}),
          addr_type_(0),
          use_L1_workspace_(0),
          L1_addr_flag_(-1),
          dtype_(dtype),
          stype_(stype),
          L1_fusion_type_(-1),
          addr_offset_(0),
          split_index_(0),
          L1_workspace_size_(-1),
          L1_valid_size_(-1),
          is_first_layer_(std::tuple<bool, bool>(false, false))
    {
        is_const_ = false;
    };

    bool GetName(std::string& name) const
    {
        name = name_;
        return true;
    };

    bool SetShape(std::vector<int64_t> shape)
    {
        shape_.assign(shape.begin(), shape.end());
        for (auto item : shape) {
            if (item < 0) {
                is_dynamic_ = true;
                break;
            }
        }
        return true;
    };

    bool GetShape(std::vector<int64_t>& shape) const
    {
        shape = shape_;
        return true;
    };

    bool SetShapeRange(const std::vector<std::pair<int64_t, int64_t>> &shapeRange)
    {
        shapeRange_ = std::make_tuple(true, shapeRange);
        return true;
    }

    bool GetShapeRange(std::vector<std::pair<int64_t, int64_t>> &shapeRange) const
    {
        if (!std::get<0>(shapeRange_)) {
            return false;
        }

        shapeRange = std::get<1>(shapeRange_);
        return true;
    }

    bool SetOriginShape(std::vector<int64_t> originShape)
    {
        origin_shape_.assign(originShape.begin(), originShape.end());
        return true;
    };

    bool GetOriginShape(std::vector<int64_t>& originShape) const
    {
        if (is_sliced_by_sgt_) {
            originShape = sgt_ori_slice_shape_;
            return true;
        }
        originShape = origin_shape_;
        return true;
    };

    bool SetOriginShapeRange(const std::vector<std::pair<int64_t, int64_t>> &shapeRange)
    {
        originShapeRange_ = std::make_tuple(true, shapeRange);
        return true;
    }

    bool GetOriginShapeRange(std::vector<std::pair<int64_t, int64_t>> &shapeRange) const
    {
        if (!std::get<0>(originShapeRange_)) {
            return false;
        }

        shapeRange = std::get<1>(originShapeRange_);
        return true;
    }

    bool SetType(std::string  dtype)
    {
        dtype_ = dtype;
        return true;
    };

    bool GetType(std::string&  dtype) const
    {
        dtype = dtype_;
        return true;
    };

    bool GetShapeType(ATTR_SHAPETYPE& stype) const
    {
        stype = stype_;
        return true;
    };

    bool SetShapeType(ATTR_SHAPETYPE stype)
    {
        stype_ = stype;
        return true;
    };

    bool SetFormat(std::string format)
    {
        format_ = format;
        return true;
    };

    bool GetFormat(std::string& format) const
    {
        format = format_;
        return true;
    };

    bool SetSubFormat(int32_t sub_format)
    {
        sub_format_ = sub_format;
        return true;
    };

    bool GetSubFormat(int32_t& sub_format) const
    {
        sub_format = sub_format_;
        return true;
    };

    bool SetOriginFormat(std::string originFormat)
    {
        origin_format_ = originFormat;
        return true;
    };

    bool GetOriginFormat(std::string& originFormat) const
    {
        originFormat = origin_format_;
        return true;
    };

    void SetConstValue(TbeAttrValue& value)
    {
        is_const_ = true;
        const_value_ = value;
    };

    bool GetConstFlag(bool& bl) const
    {
        bl = is_const_;
        return true;
    };

    bool GetConstValue(TbeAttrValue& value) const
    {
        value = const_value_;
        return true;
    };

    void SetConstValueRange(TbeAttrValue &valueRange)
    {
        is_const_value_range_ = true;
        const_value_range_ = valueRange;
    };

    void GetConstValueRangeFlag(bool &isConstValueRange) const
    {
        isConstValueRange = is_const_value_range_;
    };

    void GetConstValueRange(TbeAttrValue &valueRange) const
    {
        valueRange = const_value_range_;
    };

    void SetConstValueNone(bool isValueNone)
    {
        is_const_value_none_ = isValueNone;
    };

    void IsConstValueNone(bool &isConstValueNone) const
    {
        isConstValueNone = is_const_value_none_;
    };

    bool GetAddrType(size_t& value) const
    {
        value = addr_type_;
        return true;
    };

    bool SetAddrType(size_t value)
    {
        switch (value) {
            case RT_MEMORY_HBM:
                addr_type_ = TBE_MEMORY_DDR;
                break;
            case RT_MEMORY_L1:
                addr_type_ = TBE_MEMORY_L1;
                break;
            case RT_MEMORY_L2:
                addr_type_ = TBE_MEMORY_L2;
                break;
            default:
                addr_type_ = TBE_MEMORY_DDR;
                break;
        }
        return true;
    };

    bool SetValidShape(std::vector<int64_t> shape)
    {
        valid_shape_.assign(shape.begin(), shape.end());
        return true;
    };

    bool GetValidShape(std::vector<int64_t>& shape) const
    {
        shape = valid_shape_;
        return true;
    };

    bool SetSgtSliceShape(std::vector<int64_t> shape)
    {
        sgt_slice_shape_.assign(shape.begin(), shape.end());
        return true;
    };

    bool GetSgtSliceShape(std::vector<int64_t>& shape) const
    {
        shape = sgt_slice_shape_;
        return true;
    };

    bool SetOriSgtSliceShape(std::vector<int64_t> shape)
    {
        sgt_ori_slice_shape_.assign(shape.begin(), shape.end());
        return true;
    };

    bool GetOriSgtSliceShape(std::vector<int64_t>& shape) const
    {
        shape = sgt_ori_slice_shape_;
        return true;
    };

    bool SetIsSlicedBySgt(bool isSlicedBySgt)
    {
        is_sliced_by_sgt_ = isSlicedBySgt;
        return true;
    };

    bool GetIsSlicedBySgt(bool& isSlicedBySgt) const
    {
        isSlicedBySgt = is_sliced_by_sgt_;
        return true;
    };

    bool SetSliceOffset(std::vector<int64_t> offset)
    {
        slice_offset_.assign(offset.begin(), offset.end());
        return true;
    };

    bool GetSliceOffset(std::vector<int64_t>& offset) const
    {
        offset = slice_offset_;
        return true;
    };

    bool SetL1WorkspaceFlag(size_t value)
    {
        use_L1_workspace_ = value;
        return true;
    };

    bool GetL1WorkspaceFlag(size_t &value) const
    {
        value = use_L1_workspace_;
        return true;
    };

    bool SetL1AddrFlag(int64_t value)
    {
        L1_addr_flag_ = value;
        return true;
    };

    bool GetL1AddrFlag(int64_t &value) const
    {
        value = L1_addr_flag_;
        return true;
    };

    bool GetL1FusionType(int32_t& L1Type) const
    {
        L1Type = L1_fusion_type_;
        return true;
    };

    bool SetL1FusionType(int32_t L1Type)
    {
        L1_fusion_type_ = L1Type;
        return true;
    };

    bool SetAddrOffset(int64_t offset)
    {
        addr_offset_ = offset;
        return true;
    }

    bool GetAddrOffset(int64_t &offset) const
    {
        offset =  addr_offset_;
        return true;
    }

    bool SetTotalShape(std::vector<uint32_t>& shape)
    {
        total_shape_ = shape;
        return true;
    }

    bool GetTotalShape(std::vector<uint32_t>& shape) const
    {
        shape = total_shape_;
        return true;
    }

    bool SetSplitIndex(uint32_t index)
    {
        split_index_ = index;
        return true;
    }

    bool GetSplitIndex(uint32_t &index) const
    {
        index = split_index_;
        return true;
    }

    bool SetL1WorkspaceSize(int64_t l1size)
    {
        L1_workspace_size_ = l1size;
        return true;
    }

    bool GetL1WorkspaceSize(int64_t &l1size) const
    {
        l1size = L1_workspace_size_;
        return true;
    }

    bool SetL1ValidSize(int64_t l1size)
    {
        L1_valid_size_ = l1size;
        return true;
    }

    bool GetL1ValidSize(int64_t &l1size) const
    {
        l1size = L1_valid_size_;
        return true;
    }

    bool SetFirstLayer(bool isFirstLayer)
    {
        is_first_layer_ = std::make_tuple(true, isFirstLayer);
        return true;
    }

    bool GetFirstLayer(bool &isFirstLayer) const
    {
        if (!std::get<0>(is_first_layer_)) {
            // this parameter has not been set
            return false;
        }

        isFirstLayer = std::get<1>(is_first_layer_);
        return true;
    }

    bool SetValueRange(const std::vector<std::pair<int64_t, int64_t>> &value_range)
    {
        value_range_ = value_range;
        return true;
    }

    bool GetValueRange(std::vector<std::pair<int64_t, int64_t>> &value_range) const
    {
        if (value_range_.empty()) {
            return false;
        }

        value_range = value_range_;
        return true;
    }

    bool operator==(TbeOpTensor& rObject);

    bool is_dynamic_ = false;

private:
    // need to adapt operator== func while adding new variable
    std::string name_{""};
    // current shape and format
    std::vector<int64_t> shape_;
    std::tuple<bool, std::vector<std::pair<int64_t, int64_t>>> shapeRange_;

    std::string format_{""};
    int32_t sub_format_;

    // original shape and format
    std::vector<int64_t> origin_shape_;
    std::tuple<bool, std::vector<std::pair<int64_t, int64_t>>> originShapeRange_;

    std::string origin_format_{""};

    // L1 fusion parameter
    size_t addr_type_;
    std::vector<int64_t> valid_shape_;
    std::vector<int64_t> slice_offset_;
    size_t use_L1_workspace_;
    int64_t L1_addr_flag_;

    std::string dtype_{""};
    ATTR_SHAPETYPE stype_;
    bool is_const_{false};
    bool is_const_value_none_{false};
    bool is_const_value_range_{false};
    TbeAttrValue const_value_;
    TbeAttrValue const_value_range_;
    int32_t L1_fusion_type_;
    int64_t addr_offset_;
    std::vector<uint32_t> total_shape_;
    uint32_t split_index_;
    int64_t L1_workspace_size_;
    int64_t L1_valid_size_;
    std::tuple<bool, bool> is_first_layer_;
    std::vector<std::pair<int64_t, int64_t>> value_range_;
    bool is_sliced_by_sgt_{false};
    std::vector<int64_t> sgt_slice_shape_;
    std::vector<int64_t> sgt_ori_slice_shape_;
};

/** TT_REQ: request tensor, just one tensor in one input
  * TT_OPT: option tensor, one or zero tensor in one input
  * TT_DYN: dynamic tensor, one or more tensors in one input
  */
enum TensorType {
    TT_REQ,
    TT_OPT,
    TT_DYN,
};

enum OpBuildResCode {
    OP_BUILD_SUCC,
    OP_BUILD_FAIL,
    OP_DYNSHAPE_NOT_SUPPORT
};

enum BUILD_TYPE {
    INITIALLY_BUILD    = 0x0,
    FUZZILY_BUILD      = 0x1,
    ACCURATELY_BUILD   = 0x2,
};

class TbeOpParam {
public:
    TbeOpParam()
    {
    };

    ~TbeOpParam()
    {
    };

    TbeOpParam(TensorType type, std::vector<TbeOpTensor> tensors): type_(type), tensors_(tensors)
    {
    };

    bool GetType(TensorType& type) const
    {
        type = type_;
        return true;
    };

    bool SetType(TensorType type)
    {
        type_ = type;
        return true;
    }

    bool GetTensors(std::vector<TbeOpTensor>& tensors) const
    {
        tensors = tensors_;
        return true;
    };

    bool SetTensors(std::vector<TbeOpTensor> tensors)
    {
        tensors_ = tensors;
        return true;
    };

    bool GetName(std::string& name) const
    {
        name = name_;
        return true;
    };

    bool SetName(std::string name)
    {
        name_ = name;
        return true;
    };

    bool Clear()
    {
        tensors_.clear();
        return true;
    };

    bool SetTensor(TbeOpTensor& tensor)
    {
        tensors_.emplace_back(tensor);
        if (tensor.is_dynamic_) {
            contains_dynamic_ = true;
        }
        return true;
    }

    bool SetValueDepend(OP_VALUE_DEPEND value_depend)
    {
        value_depend_ = value_depend;
        return true;
    }

    OP_VALUE_DEPEND GetValueDepend() const
    {
        return value_depend_;
    }

    bool operator==(TbeOpParam& rObject);
    bool contains_dynamic_ = false;
private:
    // need to adapt operator== func while adding new variable
    TensorType type_{TT_REQ};
    std::vector<TbeOpTensor> tensors_;
    OP_VALUE_DEPEND value_depend_{VALUE_DEPEND_IGNORE};
    std::string name_;
};

class TbeOpInfo {
public:
    TbeOpInfo(std::string name,
              std::string module_name,
              std::string optype,
              std::string version,
              std::string coreType)
        : op_name_(name),
          op_module_name_(module_name),
          op_type_(optype),
          op_file_name_(""),
          op_func_name_(""),
          op_check_supported_name_(""),
          ddk_version_(version),
          core_type_(coreType),
          limited_range_(RANGE_UNLIMITED),
          kernel_name_(""),
          extra_params_(""),
          op_L1_space_(-1),
          op_unknown_shape_(false),
          op_use_int64_(false),
          is_dynamic_impl_(false),
          build_type_(ACCURATELY_BUILD),
          isDynamicRankSupport_(false)
    {
    };

    ~TbeOpInfo()
    {
    };

    bool GetName(std::string& name) const
    {
        name = op_name_;
        return true;
    };

    bool GetRealName(std::string& real_name) const
    {
      real_name = op_real_name_;
      return true;
    };

    void GetOpType(std::string &optype) const
    {
        optype = op_type_;
    };

    bool GetOpFileName(std::string& fileName) const
    {
        fileName = op_file_name_;
        return true;
    };

    bool SetOpFileName(std::string fileName)
    {
        op_file_name_ = fileName;
        return true;
    };
    bool GetOpFuncName(std::string& funcName) const
    {
        funcName = op_func_name_;
        return true;
    };

    bool SetOpFuncName(std::string funcName)
    {
        op_func_name_ = funcName;
        return true;
    };

    bool SetOpCoreType(std::string coreType)
    {
        core_type_ = coreType;
        options_[CORE_TYPE_NAME] = core_type_;
        return true;
    };

    bool GetOpCoreType(std::string &coreType)
    {
        coreType = core_type_;
        return true;
    };

    bool GetCheckSupportedFunc(std::string& checkName) const
    {
        checkName = op_check_supported_name_;
        return true;
    };

    bool SetCheckSupportedFunc(std::string checkName)
    {
        op_check_supported_name_ = checkName;
        return true;
    };

    /**
     * @brief: normalize function name
     * @param [in] funcName           op function name
     * @return [out] bool             succ or fail for normalizing
     */
    bool NormalizeFuncName(std::string& funcName) const;
    bool GetModuleName(std::string& moduleName) const;
    bool GetFuncName(std::string& funcName) const;

    bool SetRealName(const std::string& op_real_name) {
      op_real_name_ = op_real_name;
      return true;
    }

    bool GetVersion(std::string& version) const
    {
        version = ddk_version_;
        return true;
    };

    void AddInput(TbeOpParam& param)
    {
        inputs_.push_back(param);
        if (param.contains_dynamic_) {
            is_dynamic_ = true;
        }
    };

    bool GetInputs(std::vector<TbeOpParam>& inputs) const
    {
        inputs = inputs_;
        return true;
    };

    bool SetInputs(std::vector<TbeOpParam> inputs)
    {
        inputs_ = inputs;
        return true;
    };

    void AddOutput(TbeOpParam& param)
    {
        outputs_.push_back(param);
    };

    bool GetOutputs(std::vector<TbeOpParam>& outputs) const
    {
        outputs = outputs_;
        return true;
    };

    bool SetOutputs(std::vector<TbeOpParam> outputs)
    {
        outputs_ = outputs;
        return true;
    };

    void AddAttrValue(TbeAttrValue& value)
    {
        attr_values_.push_back(value);
    };

    bool GetAttrValues(std::vector<TbeAttrValue>& attr_values) const
    {
        attr_values = attr_values_;
        return true;
    };

    bool GetPattern(std::string& pattern) const
    {
        pattern = pattern_;
        return true;
    };

    bool SetPattern(std::string pattern)
    {
        pattern_ = pattern;
        return true;
    };

    bool GetOpImplMode(std::string& op_impl_mode) const
    {
        op_impl_mode = op_impl_mode_;
        return true;
    };

    bool SetOpImplMode(const std::string& op_impl_mode)
    {
        op_impl_mode_ = op_impl_mode;
        return true;
    };

    bool GetExtraParams(std::string& extra_params) const
    {
        extra_params = extra_params_;
        return true;
    };

    bool SetExtraParams(const std::string& extra_params)
    {
        extra_params_ = extra_params;
        return true;
    };

    bool IsDynamicImpl() const
    {
        return is_dynamic_impl_;
    };

    void SetDynamicImpl(const bool& is_dynamic_impl)
    {
        is_dynamic_impl_ = is_dynamic_impl;
    };

    bool GetKernelName(std::string& kernelName) const
    {
        kernelName = kernel_name_;
        return true;
    };

    bool SetKernelName(std::string kernelName)
    {
        kernel_name_ = kernelName;
        return true;
    };

    bool GetL1Space(int64_t& L1Space) const
    {
        L1Space = op_L1_space_;
        return true;
    };

    bool SetL1Space(int64_t L1Space)
    {
        op_L1_space_ = L1Space;
        return true;
    };

    void SetIsUnknownShape(bool unknown)
    {
        op_unknown_shape_ = unknown;
    };

    bool GetIsUnknownShape() const
    {
        return op_unknown_shape_;
    };

    void SetFlagUseInt64(bool use)
    {
        op_use_int64_ = use;
    };

    bool GetFlagUseInt64() const
    {
        return op_use_int64_;
    };

    bool GetOptions(map<std::string, string>& options) const
    {
        options = options_;
        return true;
    };

    bool SetOptions(map<std::string, string> options)
    {
        options_ = options;
        return true;
    };

    void SetBuildType(BUILD_TYPE build_type)
    {
        build_type_ = build_type;
    };

    BUILD_TYPE GetBuildType() const
    {
        return build_type_;
    };

    void SetLimitedRange(const std::string &limitedRange)
    {
        limited_range_ = limitedRange;
    };

    bool GetLimitedRange(std::string &limitedRange) const
    {
        limitedRange = limited_range_;
        return true;
    };

    void SetNode(const ge::NodePtr &nodePtr)
    {
        nodePtr_ = nodePtr;
    };

    bool GetNode(ge::NodePtr &nodePtr) const
    {
        nodePtr = nodePtr_;
        return true;
    };

    void SetDyRankSupFlag(const bool &isDynamicRankSupport)
    {
        isDynamicRankSupport_ = isDynamicRankSupport;
    }

    void GetDyRankSupFlag(bool &isDynamicRankSupport) const
    {
        isDynamicRankSupport = isDynamicRankSupport_;
    }

    bool operator==(TbeOpInfo& rObject);
    bool is_dynamic_ = false;

private:
    // need to adapt operator== func while adding new variable
    // parameter from upper
    std::string op_name_;
    std::string op_real_name_;
    std::string op_module_name_;
    std::string op_type_;
    std::string op_file_name_;
    std::string op_func_name_;
    std::string op_check_supported_name_;
    std::string ddk_version_;
    std::string core_type_;
    std::string limited_range_;

    // parameter from lower
    std::string kernel_name_;
    std::string pattern_;
    std::string op_impl_mode_;
    std::string extra_params_;

    // L1 fusion parameter
    int64_t op_L1_space_;
    bool op_unknown_shape_;
    bool op_use_int64_;
    bool is_dynamic_impl_;
    BUILD_TYPE build_type_;

    // op parameter
    std::vector<TbeOpParam> inputs_;
    std::vector<TbeOpParam> outputs_;
    std::vector<TbeAttrValue> attr_values_;

    // ge options args
    map<std::string, string> options_;

    // binary infos
    bool isDynamicRankSupport_;
    ge::NodePtr nodePtr_{nullptr};
};

/**
 * @brief: Tbe Initialize proccess
 * @param [in] options            ddk version
 * @return [out] bool             succ or fail for Tbe Initialize
 */
extern "C" bool TbeInitialize(const std::map<std::string, std::string> &options, bool *isSupportParallelCompilation);

/**
 * @brief: tbe finalize proccess
 * @return [out] bool             succ or fail for Tbe Finalize
 */
extern "C" bool TbeFinalize();

/**
 * @brief: get finished compilation task list
 * @return [out] list             finished compilation task list
 */
extern "C" bool WaitAllFinished(uint64_t graphId, vector<FinComTask> &tasks);

/*
 * @brief: check tbe op capability
 * @param [in] opinfo: op total parameter set
 * @param [out] IsSupport: result to support or not
 * @return bool: check process ok or not
 */
extern "C" bool CheckTbeSupported(TbeOpInfo& opinfo, CheckSupportedResult &isSupport, std::string &reason);

/**
 * @brief pre build tbe op
 * @return [out] bool                 succ or fail of prebuild
 */
extern "C" bool PreBuildTbeOp(TbeOpInfo& opinfo, uint64_t taskId, uint64_t graphId);

/**
 * @brief build fusion op
 * @param [in] teGraphNode            graph node ptr vector
 * @param [in] strategy               op optimization strategy, empty or json str with key
 *                                    module_name, object_name and object_value
 * @return [in] outputNode            recode json file path in it
 * @return [out] bool                 succ or fail of building fusion op
 */
extern "C" OpBuildResCode TeFusion(std::vector<ge::Node *> teGraphNode, ge::OpDescPtr opDesc,
                                   const std::vector<ge::NodePtr> &toBeDel,
                                   uint64_t taskId, uint64_t graphId, const std::string &strategy);

extern "C" OpBuildResCode TeFusionV(std::vector<ge::Node *> teGraphNode, ge::OpDescPtr opDesc,
                                    const std::vector<ge::NodePtr> &toBeDel, uint64_t taskId, uint64_t graphId,
                                    uint64_t sgtThreadIndex, const std::string &strategy);

/**
 * @brief select tbe op format
 * @param [in] tbeOpInfo            op info
 * @param [out] opDtypeformat       op date format
 * @return [out] bool               succ or fail
 */
extern "C" bool SelectTbeOpFormat (const TbeOpInfo &tbeOpInfo, std::string &opDtypeFormat);

/**
 * @brief clear instance
 * @return [out] bool                 succ or fail of TeFusionEnd
 */
extern "C" bool TeFusionEnd();

/**
 * @brief get op info
 * @param [in] tbeOpInfo            op info
 * @param [out] result              op segmentation information
 * @return [out] LX_QUERY_STATUS    LX_QUERY_FAIL, LX_QUERY_NOT_FOUND or LX_QUERY_SUCC
 */
extern "C" LX_QUERY_STATUS GetOpInfo(const TbeOpInfo &tbeOpInfo, std::string &result);

/**
 * @brief fuzz build tbe op
 * @param [in] tbeOpInfo            op info
 * @param [in] taskId               fuzz build task id
 * @param [in] graphId              fuzz build graph id
 * @param [in] node                 tbe op to compile
 * @return [out] OpBuildResCode                 succ or fail of building fusion op
 */
extern "C" OpBuildResCode FuzzBuildTbeOp(uint64_t taskId, uint64_t graphId, ge::Node &node);

/**
 * @brief check is this op has registered tbegeneralize func
 * @param [in] tbeOpInfo            op info
 * @param [in] hasRegisteredFunc    true or false
 * @return [out] bool               succ or fail of check
 */
extern "C" bool CheckIsTbeGeneralizeFuncRegistered(const TbeOpInfo &tbeOpInfo, bool &hasRegisteredFunc);

/**
 * @brief shape or value generalize with generalize type
 * @param [in] tbeOpInfo            op info
 * @param [in] generalize_type      generalize type
 * @param [in] nodePtr              node
 * @return [out] bool               succ or fail of check
 */
extern "C" bool TeGeneralize(const TbeOpInfo &tbeOpInfo, const TE_GENERALIZE_TYPE &generalizeType,
                             const ge::NodePtr &nodePtr);

/**
 * @brief get op specific info from tbe
 * @param [in] tbeOpInfo         op info
 * @param [in] opSpecificInfo    op specific info
 * @param [out] bool             succ or fail
 */
extern "C" bool GetOpSpecificInfo(const TbeOpInfo &tbeOpInfo, std::string &opSpecificInfo);

/**
 * @brief Check the validity of shape range by tbe
 * @param [in] tbeOpInfo                  op info
 * @param [out] isSupported               shape range is valid or not
 * @param [out] unSupportedInputIndexs    shape range unSupported input indexs
 * @param [out] bool                      succ or fail
 */
extern "C" bool DynamicShapeRangeCheck(const TbeOpInfo &tbeOpInfo, bool &isSupported,
                                       std::vector<size_t> &upperLimitedInputIndexs,
                                       std::vector<size_t> &lowerLimitedInputIndexs);
} // namespace te
/*lint +e148*/
#endif
