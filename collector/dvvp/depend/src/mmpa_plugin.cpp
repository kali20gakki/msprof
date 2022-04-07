#include "mmpa/mmpa_api.h"

static const char *str = "stub";

INT32 mmOpen2(const CHAR *pathName, INT32 flags, MODE mode)
{
    return 0;
}

mmSsize_t mmRead(INT32 fd, VOID *buf, UINT32 bufLen)
{
    return 0;
}

INT32 mmClose(INT32 fd)
{
    return 0;
}

CHAR *mmGetErrorFormatMessage(mmErrorMsg errnum, CHAR *buf, mmSize size)
{
    return str;
}

INT32 mmGetErrorCode()
{
    return 0;
}

INT32 mmAccess2(const CHAR *pathName, INT32 mode)
{
    return 0;
}


INT32 mmGetOptLong(INT32 argc,
                   CHAR *const *argv,
                   const CHAR *opts,
                   const mmStructOption *longOpts,
                   INT32 *longIndex)
{
    return 0;
}

INT32 mmGetOptInd()
{
    return 0;
}

CHAR *mmGetOptArg()
{
    return str;
}

INT32 mmSleep(UINT32 milliSecond)
{
    return 0;
}

INT32 mmUnlink(const CHAR *filename)
{
    return 0;
}

INT32 mmGetTid()
{
    return 0;
}

