#ifndef PNG_H
#define PNG_H

#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <arpa/inet.h>

#define PNG_SIG_SIZE 8
#define CHUNK_TYPE "rNDm"

extern const unsigned char PNG_SIG[8];

void write_chunk(FILE *f, const char *type, unsigned char *data, uint32_t len);
void encode_png(const char *input, const char *output, const char *secret, const char *pass);
void decode_png(const char *stego, const char *pass, const char *out_dir);

#endif