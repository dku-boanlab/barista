/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#pragma once

#include "common.h"
#include "event.h"
#include "mac2int.h"
#include "openflow10.h"

/** \brief The structure of Ethernet header with VLAN fields */
struct ether_vlan_header {
    uint8_t ether_dhost[ETH_ALEN];
    uint8_t ether_short[ETH_ALEN];
    uint16_t tpid;
    uint16_t tci;
    uint16_t ether_type;
} __attribute__((packed));

/** \brief The structure of ARP header */
struct arphdr {
    __be16        ar_hrd;
    __be16        ar_pro;
    unsigned char ar_hln;
    unsigned char ar_pln;
    __be16        ar_op;

    uint8_t arp_sha[ETH_ALEN];
    uint8_t arp_spa[4];
    uint8_t arp_tha[ETH_ALEN];
    uint8_t arp_tpa[4];
};

#define DHCP_SVR_PORT 67
#define DHCP_CLI_PORT 68
