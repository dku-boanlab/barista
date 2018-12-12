/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup util
 * @{
 *
 * \defgroup hash Hash Function
 * \brief Function to get a hash value using Jenkins hash
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "hash.h"

/////////////////////////////////////////////////////////////////////

#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define mix(a,b,c) \
{ \
    a -= c;  a ^= rot(c, 4);  c += b; \
    b -= a;  b ^= rot(a, 6);  a += c; \
    c -= b;  c ^= rot(b, 8);  b += a; \
    a -= c;  a ^= rot(c,16);  c += b; \
    b -= a;  b ^= rot(a,19);  a += c; \
    c -= b;  c ^= rot(b, 4);  b += a; \
}

#define final(a,b,c) \
{ \
    c ^= b; c -= rot(b,14); \
    a ^= c; a -= rot(c,11); \
    b ^= a; b -= rot(a,25); \
    c ^= b; c -= rot(b,16); \
    a ^= c; a -= rot(c,4);  \
    b ^= a; b -= rot(a,14); \
    c ^= b; c -= rot(b,24); \
}

/////////////////////////////////////////////////////////////////////

/** 
 * \brief Function to generate a hash value
 * \param data Data to make a hash value
 * \param length Data length
 * \return Generated hash value
 */
uint32_t hash_func(const uint32_t *data, size_t length)
{
    uint32_t a,b,c;

    length = length - (length % 4);

    a = b = c = 0xdeadbeef + (((uint32_t)length) << 2);

    while (length > 3) {
        a += data[0];
        b += data[1];
        c += data[2];
        mix(a,b,c);
        length -= 3;
        data += 3;
    }

    switch(length) {
    case 3:
        c += data[2];
    case 2:
        b += data[1];
    case 1:
        a += data[0];
        final(a,b,c);
        break;
    }

    return c;
}

/**
 * @}
 *
 * @}
 */
