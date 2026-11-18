/*
 * Minimal POSIX short-option parser for the native Windows UDPST client.
 * Supports grouped flags, required arguments, --, and optind=0 restart.
 * Kept private to udpst.c; not a replacement for GNU getopt extensions.
 */
#ifndef UDPST_WIN32_GETOPT_H
#define UDPST_WIN32_GETOPT_H
#include <string.h>
#include <stdio.h>

static char *optarg = NULL;
static int optind = 1;
static int opterr = 1;
static int optopt = 0;
static int udpst_win32_optpos = 1;

static int getopt(int argc, char *const argv[], const char *options)
{
    const char *spec;
    char option;
    if (optind == 0) {
        optind = 1;
        udpst_win32_optpos = 1;
    }
    optarg = NULL;
    if (optind >= argc || argv[optind] == NULL)
        return -1;
    if (udpst_win32_optpos == 1) {
        if (argv[optind][0] != '-' || argv[optind][1] == '\0')
            return -1;
        if (strcmp(argv[optind], "--") == 0) {
            ++optind;
            return -1;
        }
    }
    option = argv[optind][udpst_win32_optpos++];
    spec = strchr(options, option);
    optopt = (unsigned char)option;
    if (spec == NULL || option == ':') {
        if (opterr)
            fprintf(stderr, "unknown option -- %c\n", option);
        if (argv[optind][udpst_win32_optpos] == '\0') {
            ++optind;
            udpst_win32_optpos = 1;
        }
        return '?';
    }
    if (spec[1] == ':') {
        if (argv[optind][udpst_win32_optpos] != '\0') {
            optarg = &argv[optind][udpst_win32_optpos];
            ++optind;
        } else if (optind + 1 < argc) {
            optarg = argv[optind + 1];
            optind += 2;
        } else {
            if (opterr)
                fprintf(stderr, "option requires an argument -- %c\n", option);
            ++optind;
            udpst_win32_optpos = 1;
            return (options[0] == ':') ? ':' : '?';
        }
        udpst_win32_optpos = 1;
    } else if (argv[optind][udpst_win32_optpos] == '\0') {
        ++optind;
        udpst_win32_optpos = 1;
    }
    return (unsigned char)option;
}
#endif
