/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : serialization_helper.h
 * Description        : tuple递归展开工具
 * Author             : msprof team
 * Creation Date      : 2024/7/16
 * *****************************************************************************
 */

#ifndef ANALYSIS_INFRASTRUCTURE_DUMP_TOOLS_INCLUDE_SERIALIZATION_HELPER_H
#define ANALYSIS_INFRASTRUCTURE_DUMP_TOOLS_INCLUDE_SERIALIZATION_HELPER_H

#include <tuple>
#include <array>

namespace Analysis {

namespace Infra {

// SerializationHelper的作用是将tuple递归展开，将来升级到C++17后，可以使用std::apply代替
template<size_t N, class OStream, class Tuple>
class SerializationHelper {
public:
    static void DumpInJsonFormat(OStream &oStream,
                         const std::array<const char*, std::tuple_size<typename std::decay<Tuple>::type>{}>& columns,
    const Tuple &tp)
    {
        static_assert(N <= MAX_TUPLE_SIZE, "tuple size greater than SerializationHelper::MAX_TUPLE_SIZE");
        SerializationHelper<N - 1, OStream, Tuple>::DumpInJsonFormat(oStream, columns, tp);
        oStream[columns[N - 1]] << std::get<N - 1>(tp);
    }

    static void DumpInCsvFormat(OStream &oStream, const Tuple &tp)
    {
        static_assert(N <= MAX_TUPLE_SIZE, "tuple size greater than SerializationHelper::MAX_TUPLE_SIZE");
        SerializationHelper<N - 1, OStream, Tuple>::DumpInCsvFormat(oStream, tp);
        oStream << std::get<N - 1>(tp);
        size_t tpSize = std::tuple_size<typename std::remove_reference<Tuple>::type>();
        if (N == tpSize) {  // C++11不支持 if constexpr，将来升级后，这里可以改进
            oStream << "\r\n";
        } else {
            oStream << ',';
        }
    }
private:
    constexpr static size_t MAX_TUPLE_SIZE = 64;
};

template<class OStream, class Tuple>
class SerializationHelper<0, OStream, Tuple> {
public:
    static void DumpInJsonFormat(OStream&, const std::array<const char*,
                                 std::tuple_size<typename std::decay<Tuple>::type>{}>&,
                                 const Tuple &) {}
    static void DumpInCsvFormat(OStream &oStream, const Tuple &) {}
};

}

}

#endif