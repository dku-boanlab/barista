/*
 * Copyright 2015-2018 NSSLab, KAIST
 */

/**
 * \ingroup util
 * @{
 *
 * \defgroup str Configuration File Processing Functions
 * \brief Functions to process configuration files
 * @{
 */

/**
 * \file
 * \author Jaehyun Nam <namjh@kaist.ac.kr>
 */

#include "str.h"

/////////////////////////////////////////////////////////////////////

#define LINE_FOREACH(line, llen, buf, blen) \
    for(line = buf, \
            llen = ((strchr(line, '\n') == NULL) ? (buf + blen - line) \
                : strchr(line, '\n') - line); \
            line + llen < buf + blen; \
            line += llen + 1, \
            llen = ((strchr(line, '\n') == NULL) ? (buf + blen - line) \
                : strchr(line, '\n') - line))

/////////////////////////////////////////////////////////////////////

/** 
 * \brief Function to parse a string and then make arguments 
 * \param str Raw string
 * \param argc Integer value to store the number of arguments
 * \param argv Array to store parsed arguments
 * \param max_argc The maximum number of arguments
 */
int str2args(char *str, int *argc, char **argv, int max_argc)
{
    uint8_t single_quotes = 0;
    uint8_t double_quotes = 0;
    uint8_t delim = 1;

    *argc = 0;

    int i;
    int len = strlen(str);
    for (i = 0; i < len; i++) {
        if (str[i] == '\'') {
            if (single_quotes)
                str[i] = '\0';
            else
                i++;
            single_quotes = !single_quotes;
            goto __non_space;
        } else if (str[i] == '\"') {
            if (double_quotes)
                str[i] = '\0';
            else
                i++;
            double_quotes = !double_quotes;
            goto __non_space;
        }

        if (single_quotes || double_quotes)
            continue;

        if (isspace(str[i])) {
            delim = 1;
            str[i] = '\0';
            continue;
        }

__non_space:
        if (delim == 1) {
            delim = 0;
            argv[(*argc)++] = &str[i];
            if (*argc > max_argc)
                break;
        }
    }

    argv[*argc] = NULL;

    return 0;
}

/**
 * \brief Function to read a raw string in a file
 * \param fname File name
 * \return Raw string retrieved from the file
 */
char *str_read(char *fname)
{
    size_t hav_read = 0;
    FILE *fp = fopen(fname, "r");
    if (fp == NULL)
        return NULL;

    // get the size of a file
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    char *file = calloc(1, size + 1);
    if (file == NULL)
        return NULL;

    file[size] = '\0';

    while ((hav_read += fread(file, 1, size, fp)) < size);

    fclose(fp);

    return file;
}

/**
 * \brief Function to remove all comments and new lines
 * \param raw Raw string
 * \return Pre-processed string
 */
char *str_preproc(char *raw)
{
    char *line;
    int llen;

    int len = strlen(raw);

    LINE_FOREACH(line, llen, raw, len) {
        int i, iscomment = 0;
        for (i = 0; i < llen; i++) {
            if (!iscomment && line[i] == '#')
                iscomment = 1;
            if (iscomment)
                line[i] = ' ';

            else if (line[i] == '\\' && line[i + 1] == '\n')
                line[i + 1] = line[i] = ' ';
        }
    }

    return raw;
}

/**
 * @}
 *
 * @}
 */
