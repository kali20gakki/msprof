/**
 * @log_zip.c
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "log_zip.h"
#include "securec.h"
#include "dlog_error_code.h"

#if defined(HARDWARE_ZIP)
/*
 * @brief HardwareZipEnd: compress deinit
 * @param [in]zipStream: stream data struct
 */
STATIC void HardwareZipEnd(struct zip_stream *zipStream)
{
    if (zipStream == NULL) {
        SELF_LOG_WARN("[input] zipStream is invalid.");
        return;
    }
    int ret = hw_deflateEnd(zipStream);
    if (ret != HZIP_OK) {
        SELF_LOG_WARN("[input] zipStream deinit failed, ret=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
        return;
    }
}

/*
 * @brief HardwareZipInit: compress init by gzip
 * @param [in]zipStream: zip stream data struct
 * @param [in]sourceLen: source length
 * @param [out]dest: 2nd ptr point to destination
 * @param [out]destAvailLen: ptr point to available destination length
 * @return: SUCCESS: succeed; others: failed;
 */
STATIC LogRt HardwareZipInit(struct zip_stream *zipStream, int sourceLen, char **dest, int *destAvailLen)
{
    if ((zipStream == NULL) || (dest == NULL) || (destAvailLen == NULL)) {
        SELF_LOG_WARN("[input] zipStream, dest or destAvailLen is invalid.");
        return ARGV_NULL;
    }

    /* deflate for gzip data */
    int ret = hw_deflateInit2_(zipStream, HZIP_LEVEL_DEFAULT, HZIP_METHOD_DEFAULT, HZIP_WINDOWBITS_GZIP,
                               HZIP_MEM_LEVEL_DEFAULT, HZIP_STRATEGY_DEFAULT, HZIP_VERSION,
                               (int)sizeof(struct zip_stream));
    if (ret != HZIP_OK) {
        SELF_LOG_WARN("[input] zipStream init failed, ret=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
        return HARDWARE_ZIP_INIT_ERR;
    }

    // malloc for dest
    int dstTmpLength = sourceLen * ZIP_RATIO;
    char *dstTmp = (char *)malloc(dstTmpLength);
    if (dstTmp == NULL) {
        SELF_LOG_WARN("malloc for dest failed, strerr=%s", strerror(ToolGetErrorCode()));
        HardwareZipEnd(zipStream);
        return MALLOC_FAILED;
    }
    (void)memset_s(dstTmp, dstTmpLength, 0, dstTmpLength);

    // copy head data to dest
    ret = memcpy_s(dstTmp, dstTmpLength - 1, ST_GZIP_HEADER, ST_GZIP_HEADER_SZ);
    if (ret != EOK) {
        SELF_LOG_WARN("memcpy_s failed, result=%d, strerr=%s.", ret, strerror(ToolGetErrorCode()));
        XFREE(dstTmp);
        HardwareZipEnd(zipStream);
        return STR_COPY_FAILED;
    }
    *dest = dstTmp;
    *destAvailLen = dstTmpLength - ST_GZIP_HEADER_SZ;
    return SUCCESS;
}

/*
 * @brief HardwareZipInputCheck: check input parameter
 * @param [in]source: origin buffer
 * @param [in]sourceLen: origin buffer length
 * @param [in]dest: 2nd ptr point to output buffer
 * @param [in]destLen: ptr point to output buffer length
 * @return: SUCCESS: succeed; others: failed;
 */
STATIC LogRt HardwareZipInputCheck(const char *source, int sourceLen, const char **dest, const int *destLen)
{
    if ((source == NULL) || (dest == NULL) || (destLen == NULL)) {
        SELF_LOG_WARN("[input] input is invalid.");
        return ARGV_NULL;
    }
    if ((sourceLen <= 0) || (sourceLen > (INT_MAX / ZIP_RATIO))) {
        SELF_LOG_WARN("[input] input(sourceLen=%d) is invalid.", sourceLen);
        return INPUT_INVALID;
    }
    return SUCCESS;
}

/*
 * @brief HardwareSrcDataCopy: source data copy to zip stream
 * @param [in]source: origin buffer
 * @param [in]sourceLen: origin buffer length
 * @param [out]zipStream: zip stream data struct
 * @return: data copy length, if failed, return INVALID(-1)
 */
STATIC int HardwareSrcDataCopy(const char *source, int *sourceLen, struct zip_stream *zipStream)
{
    int ret;
    int length = INVALID;
    if (*sourceLen <= 0) {
        SELF_LOG_WARN("input is invalid, sourceLen=%d.", *sourceLen);
        return INVALID;
    }

    if (*sourceLen > BLOCK_SIZE) {
        ret = memcpy_s(zipStream->next_in, BLOCK_SIZE, source, BLOCK_SIZE);
        if (ret != EOK) {
            SELF_LOG_WARN("memcpy failed, strerr=%s.", strerror(ToolGetErrorCode()));
            return INVALID;
        }
        length = BLOCK_SIZE;
        zipStream->avail_in = BLOCK_SIZE;
        *sourceLen -= BLOCK_SIZE;
    } else {
        ret = memcpy_s(zipStream->next_in, *sourceLen, source, *sourceLen);
        if (ret != EOK) {
            SELF_LOG_WARN("memcpy failed, strerr=%s.", strerror(ToolGetErrorCode()));
            return INVALID;
        }
        length = *sourceLen;
        zipStream->avail_in = length;
        *sourceLen = 0;
    }
    return length;
}

/*
 * @brief HardwareZipProcess: compress <source> to <dest>
 * @param [in]zipStream: origin buffer length
 * @param [in]flush: flush flag
 * @param [out]dest: ptr point to output buffer
 * @param [out]destAvailLen: ptr point to available output buffer length
 * @return: compress data length, if failed, return INVALID(-1)
 */
STATIC int HardwareZipProcess(struct zip_stream *zipStream, int flush, char *dest, int *destAvailLen)
{
    zipStream->avail_out = BLOCK_SIZE;
    int ret = hw_deflate(zipStream, flush);
    if ((ret != HZIP_OK) && (ret != HZIP_STREAM_END)) {
        SELF_LOG_WARN("hw_deflate failed, flush=%d, ret=%d, strerr=%s.", flush, ret, strerror(ToolGetErrorCode()));
        return INVALID;
    }

    if (zipStream->avail_out >= BLOCK_SIZE) {
        SELF_LOG_WARN("hw_deflate wrong, avail_out=%d", zipStream->avail_out);
        return INVALID;
    }

    int length = BLOCK_SIZE - zipStream->avail_out;
    if (*destAvailLen < length) {
        SELF_LOG_WARN("hw_deflate wrong, length=%d, destAvailLen=%d", zipStream->avail_out, *destAvailLen);
        return INVALID;
    }
    ret = memcpy_s(dest, *destAvailLen, zipStream->next_out, length);
    if (ret != EOK) {
        SELF_LOG_WARN("memcpy failed, strerr=%s.", strerror(ToolGetErrorCode()));
        return INVALID;
    }
    *destAvailLen -= length;
    return length;
}

/*
 * @brief HardwareZip: compress <source> to <dest>
 * @param [in]source: origin buffer
 * @param [in]sourceLen: origin buffer length
 * @param [out]dest: 2nd ptr point to output buffer
 * @param [out]destLen: ptr point to output buffer length
 * @return: SUCCESS: succeed; others: failed;
 */
LogRt HardwareZip(const char *source, int sourceLen, char **dest, int *destLen)
{
    int flush = HZIP_FLUSH_TYPE_SYNC_FLUSH;
    int destAvailLen = 0;
    char *dstTmp = NULL;
    struct zip_stream zipStream;
    (void)memset_s(&zipStream, sizeof(zipStream), 0, sizeof(zipStream));
    LogRt logRet = HardwareZipInputCheck(source, sourceLen, (const char **)dest, destLen);
    if (logRet != SUCCESS) {
        return logRet;
    }
    logRet = HardwareZipInit(&zipStream, sourceLen, &dstTmp, &destAvailLen);
    if (logRet != SUCCESS) {
        return logRet;
    }
    *dest = dstTmp;
    dstTmp += ST_GZIP_HEADER_SZ;
    *destLen = ST_GZIP_HEADER_SZ;
    do {
        int length = HardwareSrcDataCopy(source, &sourceLen, &zipStream);
        ONE_ACT_NO_LOG(length == INVALID, goto END);
        if (length == BLOCK_SIZE) {
            source += BLOCK_SIZE;
        }
        flush = (sourceLen != 0) ? HZIP_FLUSH_TYPE_SYNC_FLUSH : HZIP_FLUSH_TYPE_FINISH;
        do {
            length = HardwareZipProcess(&zipStream, flush, dstTmp, &destAvailLen);
            ONE_ACT_NO_LOG(length == INVALID, goto END);
            dstTmp += length;
            *destLen += length;
        } while (zipStream.avail_in > 0);
    } while (flush != HZIP_FLUSH_TYPE_FINISH);
    HardwareZipEnd(&zipStream);
    return SUCCESS;

END:
    HardwareZipEnd(&zipStream);
    XFREE(*dest);
    return HARDWARE_ZIP_ERR;
}

LogRt WriteLogBlock(int fd, const LogMsgHeadInfo *msgBlock, const char *logData)
{
    ONE_ACT_WARN_LOG(fd <= 0, return ARGV_NULL, "[input] file handle is invalid.");
    ONE_ACT_WARN_LOG(msgBlock == NULL, return ARGV_NULL, "[input] log head info is null.");
    ONE_ACT_WARN_LOG(logData == NULL, return ARGV_NULL, "[input] log data is null.");

    int bytes = ToolWrite(fd, logData, msgBlock->data_len); // write log data
    if (bytes == INVALID) {
        SELF_LOG_WARN("write log data failed, dataLen=%d, writen=%d, strerr=%s.", msgBlock->data_len, bytes,
                      strerror(ToolGetErrorCode()));
        return WRITE_FILE_ERR;
    }
    return SUCCESS;
}
#endif // HARDWARE_ZIP

STATIC unsigned short g_zipSwitch = ZIP_OFF;

void GetZipCfg(void)
{
    char argvTmp[CONF_VALUE_MAX_LEN + 1] = { 0 };
    // get zip switch
    LogRt ret = GetConfValueByList(ZIP_SWITCH, strlen(ZIP_SWITCH), argvTmp, CONF_VALUE_MAX_LEN);
    if (ret == SUCCESS) {
        if (IsNaturalNumStr(argvTmp)) {
            int zipSwitch = atoi(argvTmp);
            if ((zipSwitch == ZIP_ON) || (zipSwitch == ZIP_OFF)) {
                g_zipSwitch = zipSwitch;
            } else {
                SELF_LOG_WARN("zip_switch is illegal, use default(%u).", g_zipSwitch);
            }
        } else {
            SELF_LOG_WARN("zip_switch is illegal, use default(%u).", g_zipSwitch);
        }
    }
}

unsigned short IsZip(void)
{
    return g_zipSwitch;
}
