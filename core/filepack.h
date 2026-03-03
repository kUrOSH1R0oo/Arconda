#ifndef FILEPACK_H
#define FILEPACK_H

#define _GNU_SOURCE
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <endian.h>
#include <libgen.h>
#include <sys/stat.h>

unsigned char *pack_file(const char *path, size_t *out_len);
void unpack_file(unsigned char *blob, size_t blob_len, const char *out_dir);

#endif