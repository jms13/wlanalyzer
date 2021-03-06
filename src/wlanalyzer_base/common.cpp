/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Samsung Electronics
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include "common.h"

#ifndef DEBUG_BUILD
void wld_log(const char *format, ...)
{
    return;
}
#else

void wld_log(const char *format, ...)
{
    va_list vargs;
    va_start(vargs, format);

    vprintf(format, vargs);

    va_end(vargs);
}
#endif // DEBUG_BUILD

void debug_print(const char *buf)
{
    int errno_backup = errno;
    write(2, buf, strlen(buf));
    write(2, "\n", 1);
    errno = errno_backup;
}

int check_error(int error)
{
    const char *buf;

    if (error < 0)
    {
        buf = strerror(errno);
        debug_print(buf);

        return -1;
    }

    return 0;
}

uint32_t byteArrToUInt32(const char byte[])
{
    uint32_t ret = 0;

    ret = byte[0] | (byte[1] << 8) | (byte[2] << 16) | (byte[3] << 24);

    return ret;
}

void set_bit(uint32_t *val, int num, bool bit)
{
    *val = (*val & ~(1 << num)) | (bit << num);
}

bool bit_isset(const uint32_t &val, int num)
{
    return val & (1 << num);
}


uint16_t byteArrToUInt16(const char byte[])
{
    uint16_t ret = 0;

    ret = byte[0] | (byte[1] << 8);

    return ret;
}
