/*
 * tinfgzip - tiny gzip decompressor
 *
 * Copyright (c) 2003-2019 Joergen Ibsen
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must
 *      not claim that you wrote the original software. If you use this
 *      software in a product, an acknowledgment in the product
 *      documentation would be appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must
 *      not be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any source
 *      distribution.
 */

#include <tinf/tinf.h>

typedef enum {
    FTEXT    = 1,
    FHCRC    = 2,
    FEXTRA   = 4,
    FNAME    = 8,
    FCOMMENT = 16
} tinf_gzip_flag;

static unsigned int read_le16(const unsigned char *p)
{
    return ((unsigned int) p[0])
        | ((unsigned int) p[1] << 8);
}

static unsigned int read_le32(const unsigned char *p)
{
    return ((unsigned int) p[0])
        | ((unsigned int) p[1] << 8)
        | ((unsigned int) p[2] << 16)
        | ((unsigned int) p[3] << 24);
}

int tinf_gzip_uncompress(void *dest, unsigned int *destLen,
        const void *source, unsigned int sourceLen)
{
    const unsigned char *src = (const unsigned char *) source;
    unsigned char *dst = (unsigned char *) dest;
    const unsigned char *start;
    unsigned int dlen, crc32;
    int res;
    unsigned char flg;

    /* -- Check header -- */

    /* Check room for at least 10 byte header and 8 byte trailer */
    if (sourceLen < 18) {
        //printf("File too small\n");
        return TINF_DATA_ERROR;
    }

    /* Check id bytes */
    if (src[0] != 0x1F || src[1] != 0x8B) {
        //printf("ID bytes aren't correct\n");
        return TINF_DATA_ERROR;
    }

    /* Check method is deflate */
    if (src[2] != 8) {
        //printf("Method isn't deflate\n");
        return TINF_DATA_ERROR;
    }

    /* Get flag byte */
    flg = src[3];

    /* Check that reserved bits are zero */
    if (flg & 0xE0) {
        //printf("Reserved bits aren't zero\n");
        return TINF_DATA_ERROR;
    }

    /* -- Find start of compressed data -- */

    /* Skip base header of 10 bytes */
    start = src + 10;

    /* Skip extra data if present */
    if (flg & FEXTRA) {
        unsigned int xlen = read_le16(start);

        if (xlen > sourceLen - 12) {
            return TINF_DATA_ERROR;
        }

        start += xlen + 2;
    }

    /* Skip file name if present */
    if (flg & FNAME) {
        do {
            if (start - src >= sourceLen) {
                //printf("File name\n");
                return TINF_DATA_ERROR;
            }
        } while (*start++);
    }

    /* Skip file comment if present */
    if (flg & FCOMMENT) {
        do {
            if (start - src >= sourceLen) {
                //printf("File comment\n");
                return TINF_DATA_ERROR;
            }
        } while (*start++);
    }

    /* Check header crc if present */
    if (flg & FHCRC) {
        unsigned int hcrc;

        if (start - src > sourceLen - 2) {
            //printf("Headder crc 1\n");
            return TINF_DATA_ERROR;
        }

        hcrc = read_le16(start);

        if (hcrc != (tinf_crc32(src, start - src) & 0x0000FFFF)) {
            //printf("Headder crc 2\n");
            return TINF_DATA_ERROR;
        }

        start += 2;
    }

    /* -- Get decompressed length -- */

    dlen = read_le32(&src[sourceLen - 4]);

    if (dlen > *destLen) {
        //printf("Decompressed length is larger than destLen\n");
        return TINF_BUF_ERROR;
    }

    /* -- Get CRC32 checksum of original data -- */

    crc32 = read_le32(&src[sourceLen - 8]);

    /* -- Decompress data -- */

    if ((src + sourceLen) - start < 8) {
        return TINF_DATA_ERROR;
    }

    res = tinf_uncompress(dst, destLen, start,
            (src + sourceLen) - start - 8);

    if (res != TINF_OK) {
        return TINF_DATA_ERROR;
    }

    if (*destLen != dlen) {
        return TINF_DATA_ERROR;
    }

    /* -- Check CRC32 checksum -- */

    if (crc32 != tinf_crc32(dst, dlen)) {
        return TINF_DATA_ERROR;
    }

    return TINF_OK;
}
