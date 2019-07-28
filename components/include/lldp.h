/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \file
 * \author Yeonkeun Kim <yeonk@kaist.ac.kr>
 */

#pragma once

/** \brief LLDP ethernet type */
#define ETHERTYPE_LLDP 0x88cc

/** \brief The default TTL value */
#define DEFAULT_TTL 64

struct tlv_header {
    union {
        struct {
            uint16_t length:9;
            uint16_t type:7; /* One of the LLDP_TLVT_ */
        };
        uint16_t raw;
    };
    uint8_t data[0];         /* 0 ~ 511 bytes */
};

enum lldp_tlv_type {
    /* Mandatory */
    LLDP_TLVT_EOL = 0,      /* End of LLDPDU */
    LLDP_TLVT_CHASSISID,    /* Chassis ID */
    LLDP_TLVT_PORTID,       /* Port ID */
    LLDP_TLVT_TTL,          /* Time To Live */

    /* Optional */
    LLDP_TLVT_PORT_DESC,    /* Port Description */
    LLDP_TLVT_SYSTEM_NAME,  /* System Name */
    LLDP_TLVT_SYSTEM_DESC,  /* System Description */
    LLDP_TLVT_SYSTEM_CAPA,  /* System Capabilities */
    LLDP_TLVT_MGMT_ADDR,    /* Management Address */

    LLDP_TLVT_ORGANIZ = 127 /* Organizationally Specific TLVs */
};

typedef struct _lldp_eol {
    struct tlv_header hdr;
} lldp_eol;

enum lldp_chassis_id_subtype {
    LLDP_CHASSISID_SUBTYPE_COMPONENT = 1,
    LLDP_CHASSISID_SUBTYPE_IFALIAS,
    LLDP_CHASSISID_SUBTYPE_PORT_COMPONENT,
    LLDP_CHASSISID_SUBTYPE_MACADDR,
    LLDP_CHASSISID_SUBTYPE_NETADDR,
    LLDP_CHASSISID_SUBTYPE_IFNAME,
    LLDP_CHASSISID_SUBTYPE_LOCAL
};

typedef struct _lldp_chassis_id {
    struct tlv_header hdr;
    uint8_t subtype;  /* One of the LLDP_CHASSISID_SUBTYPE_ */
    uint8_t data[0];  /* 1 ~ 255 bytes */
} lldp_chassis_id;

enum lldp_port_id_subtype {
    LLDP_PORTID_SUBTYPE_IFALIAS = 1,
    LLDP_PORTID_SUBTYPE_COMPONENT,
    LLDP_PORTID_SUBTYPE_MACADDR,
    LLDP_PORTID_SUBTYPE_NETADDR,
    LLDP_PORTID_SUBTYPE_IFNAME,
    LLDP_PORTID_SUBTYPE_AGENTCID,
    LLDP_PORTID_SUBTYPE_LOCAL
};

typedef struct _lldp_port_id {
    struct tlv_header hdr;
    uint8_t subtype;  /* One of the LLDP_PORTID_SUBTYPE_ */
    uint8_t data[0];  /* 1 ~ 255 bytes */
} lldp_port_id;

typedef struct _lldp_ttl {
    struct tlv_header hdr;
    uint16_t ttl;
} lldp_ttl;

typedef struct _lldp_system_desc {
    struct tlv_header hdr;
    uint8_t data[0];
} lldp_system_desc;
