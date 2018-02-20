/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup util
 * @{
 *
 * \defgroup str Search Function
 * \brief Function to find external configuration files
 * @{
 */

/**
 * \file
 * \author Hyeonseong Jo <hsjjo@kaist.ac.kr>
 */

#include "search.h"

#include <dirent.h>

/////////////////////////////////////////////////////////////////////

/**
 * \brief Function to find configuration files
 * \param temp The paths of the files that will be discovered
 * \param cnt The number of the files that will be discovered
 * \param curr The current directory
 */
int get_external_configurations(char (*temp)[__CONF_WORD_LEN], int *cnt, char *curr)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;

    if (!(dir = opendir(curr))) {
        return -1;
    } else if (!(entry = readdir(dir))) {
        closedir(dir);
        return -1;
    }

    do {
        char path[__CONF_WORD_LEN] = {0};
        int len = snprintf(path, sizeof(path)-1, "%s/%s", curr, entry->d_name);
        path[len] = '\0';

        if (entry->d_type == DT_DIR) {
            if(strncmp(entry->d_name, ".", 1) == 0 || strncmp(entry->d_name, "..", 2) == 0)
                continue;
            get_external_configurations(temp, cnt, path);
        } else {
            if (strstr(entry->d_name, ".conf") != NULL) {
                strcpy(temp[(*cnt)], path);
                (*cnt)++;
            }
        }
    } while ((entry = readdir(dir)) != NULL);

    closedir(dir);

    return 0;
}

/**
 * @}
 *
 * @}
 */
