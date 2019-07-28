/*
 * Copyright 2015-2019 NSSLab, KAIST
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

static int ODP_FUNC(odp_t *odp, const ODP_TYPE *data)
{
    int i, j, pass = TRUE; // assume that there is no matched policy

    if (odp[0].flag == 0) return FALSE; // there is no policy

    for (i=0; i<__MAX_POLICIES; i++) {
        if (odp[i].flag == 0) break; // No more ODP

        int cnt = 0, match = 0;

        if (odp[i].flag & ODP_DPID) {
            cnt++;

            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (odp[i].dpid[j] == 0) break; // No more DPID

                if (odp[i].dpid[j] == data->dpid) {
                    match++;
                    break;
                }
            }
        }

        if (odp[i].flag & ODP_PORT) {
            cnt++;

            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (odp[i].port[j] == 0) break; // No more port

                if (odp[i].port[j] == data->port) {
                    match++;
                    break;
                }
            }
        }

        if (odp[i].flag & ODP_PROTO) {
            cnt++;

            if (odp[i].proto & data->pkt_info.proto)
                match++;
        } 

        if (odp[i].flag & ODP_VLAN) {
            cnt++;

            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (odp[i].vlan[j] == 0) break; // No more VLAN

                if (odp[i].vlan[j] == data->pkt_info.vlan_id) {
                    match++;
                    break;
                }
            }
        }

        if (odp[i].flag & ODP_SRCIP) {
            cnt++;

            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (odp[i].srcip[j] == 0) break; // No more src_ip

                if ((odp[i].srcip[j] & data->pkt_info.src_ip) == odp[i].srcip[j]) {
                    match++;
                    break;
                }
            }
        }

        if (odp[i].flag & ODP_DSTIP) {
            cnt++;

            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (odp[i].dstip[j] == 0) break; // No more dst_ip

                if ((odp[i].dstip[j] & data->pkt_info.dst_ip) == odp[i].dstip[j]) {
                    match++;
                    break;
                }
            }
        }

        if (odp[i].flag & ODP_SPORT) {
            cnt++;

            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (odp[i].sport[j] == 0) break; // No more sport

                if (odp[i].sport[j] == data->pkt_info.src_port) {
                    match++;
                    break;
                }
            }
        }

        if (odp[i].flag & ODP_DPORT) {
            cnt++;

            for (j=0; j<__MAX_POLICY_ENTRIES; j++) {
                if (odp[i].dport[j] == 0) break; // No more dport

                if (odp[i].dport[j] == data->pkt_info.dst_port) {
                    match++;
                    break;
                }
            }
        }

        if (cnt == match) { // found one matched policy
            pass = FALSE;
            break;
        }
    }

    return pass;
}
