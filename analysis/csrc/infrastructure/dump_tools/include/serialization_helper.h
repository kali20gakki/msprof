/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

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
    static void DumpInCsvFormat(OStream &, const Tuple &) {}
};

}

}

#endif