#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <zlib.h>

void derive_key(const char *pass, unsigned char *salt, unsigned char *key);
unsigned char *encrypt_payload(unsigned char *data, size_t data_len, const char *pass, size_t *out_len);
unsigned char *decrypt_payload(unsigned char *blob, size_t blob_len, const char *pass, size_t *out_len);

#endif