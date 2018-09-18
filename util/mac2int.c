/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup util
 * @{
 *
 * \defgroup mac2int MAC-to-Int Functions
 * \brief Functions to convert a MAC address to an integer and vice versa
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "mac2int.h"

/**
 * \brief Function to convert the byte array of a MAC address to an integer
 * \param mac The pointer of a byte array
 * \return The integer converted from the byte array
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
 * \brief Function to convert the integer value of a MAC address to a byte array
 * \param mac The integer value of a MAC address
 * \param macaddr The pointer of a byte array to store the converted result
 * \return The byte array converted from the integer
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
 * \brief Function to convert the string of a MAC address to a byte array
 * \param macaddr The string of a MAC address
 * \param mac The pointer of a byte array to store the converted result
 * \return The byte array converted from the string
 */
uint8_t *str2mac(const char *macaddr, uint8_t *mac)
{
    int tmp[ETH_ALEN] = {0};

    sscanf(macaddr, "%x:%x:%x:%x:%x:%x", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5]);

    int i;
    for (i=0; i<ETH_ALEN; i++)
        mac[i] = (uint8_t)tmp[i];

    return mac;
}

/**
 * \brief Function to convert the byte array of a MAC address to a string
 * \param mac The pointer of a byte array
 * \param macaddr The pointer of a string to store the converted result
 * \return The string converted from the byte array
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
