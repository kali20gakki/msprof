/**
 * @file adx_msg.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_MSG_H
#define ADX_MSG_H
#include <cstdint>
namespace Adx {
const uint16_t ADX_PROTO_MAGIC_VALUE         = 0xC396;
const uint8_t ADX_PROTO_VERSION              = 0x10;      // version v1.0
enum class MsgType:uint16_t {
    MSG_DATA,
    MSG_CTRL,
    NR_MSG_TYPE,
};

enum class MsgStatus:uint16_t {
    // contrl status
    MSG_STATUS_HAND_SHAKE,
    MSG_STATUS_DATA_IN,           // in transfering data
    MSG_STATUS_DATA_END,          // transfering end message
    // msg status                 // data dump remote message
    MSG_STATUS_FILE_LOAD,         // begin load file
    MSG_STATUS_LOAD_ERROR,        // load file exception
    MSG_STATUS_LOAD_DONE,         // load file done
    // return status
    MSG_STATUS_NONE_ERROR,        // none
    MSG_STATUS_FILE_ERROR,        // file
    MSG_STATUS_MEMORY_ERROR,      // opreate memory error
    MSG_STATUS_NO_SPACE_ERROR,    // no disk space
    MSG_STATUS_PREMISSION_ERROR,  // permission deny
    MSG_STATUS_CACHE_FULL_ERROR,  // cache full
    NR_MSG_STATUS
};

struct MsgProto {
    uint16_t headInfo;    // head magic data, judge to little,
    uint8_t headVer;      // head version
    uint8_t order;        // packet order (reserved)
    uint16_t reqType;     // request type of proto
    uint16_t devId;       // request device Id
    uint32_t totalLen;    // whole message length, only all data[0] length
    uint32_t sliceLen;    // one slice length, only data[0] length
    uint32_t offset;      // offset
    MsgType msgType;     // message type
    MsgStatus status;      // message status data
    uint8_t data[0];      // message data
};
}
#endif