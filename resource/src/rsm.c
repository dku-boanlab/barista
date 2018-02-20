/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \defgroup extensions Extensions
 * @{
 * \defgroup resource Resource Monitor
 * \brief Resource monitor for the resource management
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "rsm.h"

/**
 * \brief Function to monitor CPU and memory usages
 * \param argc The number of arguments
 * \param argv Arguments
 */
int main(int argc, char **argv)
{
    int num_proc = get_nprocs();

    char cmd[__CONF_WORD_LEN] = {0};
    sprintf(cmd, "ps -p $(pgrep -x barista) -o %%cpu,%%mem 2> /dev/null | tail -n 1");

    while (1) {
        time_t timer;
        struct tm *tm_info;

        double cpu = 0.0, mem = 0.0;

        char now[__CONF_WORD_LEN] = {0};
        char buf[__CONF_WORD_LEN] = {0};

        time(&timer);
        tm_info = localtime(&timer);
        strftime(now, __CONF_WORD_LEN-1, "%Y:%m:%d %H:%M:%S", tm_info);

        {
            FILE *pp = popen(cmd, "r");
            if (pp != NULL) {
                char temp[__CONF_WORD_LEN] = {0};

                if (fgets(temp, __CONF_WORD_LEN-1, pp) == NULL) {
                    pclose(pp);
                    continue;
                }

                sscanf(temp, "%lf %lf", &cpu, &mem);

                pclose(pp);
            }
        }

        cpu /= num_proc; // normalization (under 100%)

        sprintf(buf, "%s %lf %lf", now, cpu, mem);

        FILE *fp = fopen(DEFAULT_RAW_RESOURCE_FILE, "w");
        if (fp != NULL) {
            fprintf(fp, "%s", buf);
            fclose(fp);
        }

        waitsec(RESOURCE_MGMT_MONITOR_TIME, 0);
    }

    return 0;
}

/**
 * @}
 *
 * @}
 */
