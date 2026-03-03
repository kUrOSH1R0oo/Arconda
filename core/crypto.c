#include "crypto.h"
#include "util.h"

void derive_key(const char *pass, unsigned char *salt, unsigned char *key) {
    if (!PKCS5_PBKDF2_HMAC(pass, strlen(pass), salt, 16, 200000, EVP_sha256(), 32, key))
        die("Key derivation failed - OpenSSL error");
}

unsigned char *encrypt_payload(unsigned char *data, size_t data_len, const char *pass, size_t *out_len) {
    unsigned char salt[16];
    unsigned char nonce[12];
    unsigned char key[32];

    if (RAND_bytes(salt, 16) != 1) die("Failed to generate random salt");
    if (RAND_bytes(nonce, 12) != 1) die("Failed to generate random nonce");

    derive_key(pass, salt, key);

    uLong comp_bound = compressBound(data_len);
    unsigned char *compressed = malloc(comp_bound);
    if (!compressed) die("Memory allocation failed for compression");

    if (compress2(compressed, &comp_bound, data, data_len, 9) != Z_OK) {
        free(compressed);
        die("Data compression failed");
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        free(compressed);
        die("Failed to create encryption context");
    }

    unsigned char *cipher = malloc(comp_bound + 16);
    if (!cipher) {
        EVP_CIPHER_CTX_free(ctx);
        free(compressed);
        die("Memory allocation failed for ciphertext");
    }

    int len, cipher_len;
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, nonce) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        free(compressed);
        free(cipher);
        die("Encryption initialization failed");
    }

    if (EVP_EncryptUpdate(ctx, cipher, &len, compressed, comp_bound) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        free(compressed);
        free(cipher);
        die("Encryption update failed");
    }
    cipher_len = len;

    if (EVP_EncryptFinal_ex(ctx, cipher + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        free(compressed);
        free(cipher);
        die("Encryption finalization failed");
    }
    cipher_len += len;

    unsigned char tag[16];
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        free(compressed);
        free(cipher);
        die("Failed to get authentication tag");
    }
    EVP_CIPHER_CTX_free(ctx);

    *out_len = 16 + 12 + cipher_len + 16;
    unsigned char *out = malloc(*out_len);
    if (!out) {
        free(compressed);
        free(cipher);
        die("Memory allocation failed for output buffer");
    }

    memcpy(out, salt, 16);
    memcpy(out + 16, nonce, 12);
    memcpy(out + 28, cipher, cipher_len);
    memcpy(out + 28 + cipher_len, tag, 16);

    free(cipher);
    free(compressed);
    return out;
}

unsigned char *decrypt_payload(unsigned char *blob, size_t blob_len, const char *pass, size_t *out_len) {
    if (blob_len < 16 + 12 + 16) die("Encrypted data is corrupted or too small");

    unsigned char *salt = blob;
    unsigned char *nonce = blob + 16;
    unsigned char *ciphertext = blob + 28;
    size_t cipher_len = blob_len - 28 - 16;
    unsigned char *tag = blob + blob_len - 16;

    unsigned char key[32];
    derive_key(pass, salt, key);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) die("Failed to create decryption context");

    unsigned char *plaintext = malloc(cipher_len);
    if (!plaintext) {
        EVP_CIPHER_CTX_free(ctx);
        die("Memory allocation failed for plaintext");
    }

    int len, plain_len;
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, nonce) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        die("Decryption initialization failed");
    }

    if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, cipher_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        die("Decryption update failed");
    }
    plain_len = len;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        die("Failed to set authentication tag");
    }

    int ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
    EVP_CIPHER_CTX_free(ctx);

    if (ret != 1) {
        free(plaintext);
        die("[-] Decryption failed (wrong passphrase or corrupted data)");
    }
    plain_len += len;

    uLongf decomp_len = plain_len * 10;
    unsigned char *decomp = malloc(decomp_len);
    if (!decomp) {
        free(plaintext);
        die("[-] Memory allocation failed for decompression");
    }

    if (uncompress(decomp, &decomp_len, plaintext, plain_len) != Z_OK) {
        free(plaintext);
        free(decomp);
        die("[-] Data decompression failed");
    }

    *out_len = decomp_len;
    free(plaintext);
    return decomp;
}
