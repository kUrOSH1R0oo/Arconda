#ifndef UTIL_H
#define UTIL_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>

void die(const char *msg);
void die_with_file(const char *msg, const char *filename);
void read_file(const char *path, unsigned char **buf, size_t *len);
void write_file(const char *path, unsigned char *buf, size_t len);

#endif