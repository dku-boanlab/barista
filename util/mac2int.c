/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \ingroup util
 * @{
 *
 * \defgroup mac2int MAC Address Conversion Functions
 * \brief Functions to convert the forms of MAC addresses
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "mac2int.h"

/**
 * \brief Function to convert a byte array to an integer value
 * \param mac Byte array
 * \return Integer value
 */
uint64_t mac2int(const uint8_t *mac)
{
    const uint8_t *p = mac;
    uint64_t res = 0;

    int i;
    for (i=5; i>=0; i--) {
        res |= (uint64_t) *p++ << (8 * i);
    }

    return res;
}

/**
 * \brief Function to convert an integer value to a byte array
 * \param mac Integer value
 * \param macaddr Byte array to store the converted result
 * \return Byte array
 * */
uint8_t *int2mac(const uint64_t mac, uint8_t *macaddr)
{
    uint8_t *p = macaddr;

    int i;
    for (i=5; i>=0; i--) {
        *p++ = mac >> (8 * i);
    }

    return macaddr;
}

/**
 * \brief Function to convert a string to a byte array
 * \param macaddr String (xx:xx:xx:xx:xx:xx)
 * \param mac Byte array to store the converted result
 * \return Byte array
 */
uint8_t *str2mac(const char *macaddr, uint8_t *mac)
{
    int tmp[ETH_ALEN];

    sscanf(macaddr, "%x:%x:%x:%x:%x:%x", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5]);

    int i;
    for (i=0; i<ETH_ALEN; i++)
        mac[i] = (uint8_t)tmp[i];

    return mac;
}

/**
 * \brief Function to convert a byte array to a string
 * \param mac Byte array
 * \param macaddr String to store the converted result
 * \return String (xx:xx:xx:xx:xx:xx)
 */
char *mac2str(const uint8_t *mac, char *macaddr)
{
    sprintf(macaddr, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return macaddr;
}

/**
 * @}
 *
 * @}
 */
