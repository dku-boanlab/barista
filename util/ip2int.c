/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup util
 * @{
 *
 * \defgroup ip2int IP address Format Converting Functions
 * \brief Functions to convert the forms of IP addresses
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "ip2int.h"

/**
 * \brief Function to convert an integer value to a string
 * \param ip Integer value
 * \return String (x.x.x.x)
 */
char *ip_addr_str(const uint32_t ip)
{
    uint8_t bytes[4] = {0};
    static char ipaddr[__CONF_WORD_LEN] = {0};

    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;

    sprintf(ipaddr, "%u.%u.%u.%u", bytes[3], bytes[2], bytes[1], bytes[0]);

    return ipaddr;
}

/**
 * \brief Function to convert a string to an integer value
 * \param ipaddr String (x.x.x.x)
 * \return Integer value
 */
uint32_t ip_addr_int(const char *ipaddr)
{
    unsigned int ipbytes[4] = {0};

    sscanf(ipaddr, "%u.%u.%u.%u", &ipbytes[3], &ipbytes[2], &ipbytes[1], &ipbytes[0]);

    return ipbytes[0] | ipbytes[1] << 8 | ipbytes[2] << 16 | ipbytes[3] << 24;
}

/**
 * @}
 *
 * @}
 */
