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

        if (cnt == match) { // found one matched policy
            pass = FALSE;
            break;
        }
    }

    return pass;
}
