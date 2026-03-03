#include "png.h"
#include "util.h"
#include "crypto.h"
#include "filepack.h"

const unsigned char PNG_SIG[8] = {0x89, 'P','N','G','\r','\n',0x1a,'\n'};

void write_chunk(FILE *f, const char *type, unsigned char *data, uint32_t len) {
    uint32_t be_len = htonl(len);
    if (fwrite(&be_len, 4, 1, f) != 1) die("[-] Failed to write chunk length");
    if (fwrite(type, 1, 4, f) != 4) die("[-] Failed to write chunk type");
    if (fwrite(data, 1, len, f) != len) die("[-] Failed to write chunk data");

    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (unsigned char*)type, 4);
    crc = crc32(crc, data, len);
    uint32_t be_crc = htonl(crc);
    if (fwrite(&be_crc, 4, 1, f) != 1) die("Failed to write chunk CRC");
}

void encode_png(const char *input, const char *output, const char *secret, const char *pass) {
    printf("[*] Reading carrier PNG: %s\n", input);
    unsigned char *png;
    size_t png_len;
    read_file(input, &png, &png_len);

    if (png_len < 8 || memcmp(png, PNG_SIG, 8) != 0) {
        free(png);
        die_with_file("File is not a valid PNG", input);
    }

    printf("[*] Packing secret file: %s\n", secret);
    size_t packed_len;
    unsigned char *packed = pack_file(secret, &packed_len);

    size_t enc_len;
    unsigned char *enc = encrypt_payload(packed, packed_len, pass, &enc_len);

    FILE *out = fopen(output, "wb");
    if (!out) {
        free(png);
        free(packed);
        free(enc);
        die_with_file("Cannot create output PNG file", output);
    }

    if (fwrite(PNG_SIG, 1, 8, out) != 8) {
        fclose(out);
        free(png);
        free(packed);
        free(enc);
        die_with_file("Failed to write PNG signature to output file", output);
    }

    size_t pos = 8;
    int chunks_written = 0;
    
    while (pos + 8 <= png_len) {
        uint32_t len;
        memcpy(&len, png + pos, 4);
        len = ntohl(len);

        if (pos + 12 + len > png_len) break;

        char type[5];
        memcpy(type, png + pos + 4, 4);
        type[4] = 0;

        unsigned char *data = png + pos + 8;

        if (strcmp(type, "IEND") == 0) {
            write_chunk(out, CHUNK_TYPE, enc, enc_len);
            chunks_written++;
        }

        write_chunk(out, type, data, len);
        chunks_written++;
        pos += 12 + len;
    }

    fclose(out);
    free(png);
    free(packed);
    free(enc);

    printf("[+] Success! File embedded into: %s (processed %d chunks)\n", output, chunks_written);
}

void decode_png(const char *stego, const char *pass, const char *out_dir) {
    printf("[*] Reading stego PNG: %s\n", stego);
    unsigned char *png;
    size_t png_len;
    read_file(stego, &png, &png_len);

    if (png_len < 8 || memcmp(png, PNG_SIG, 8) != 0) {
        free(png);
        die_with_file("File is not a valid PNG", stego);
    }

    printf("[*] Analyzing PNG structure...\n");
    size_t pos = 8;
    int found = 0;
    int chunk_count = 0;

    while (pos + 8 <= png_len) {
        uint32_t len;
        memcpy(&len, png + pos, 4);
        len = ntohl(len);

        if (pos + 12 + len > png_len) break;

        char type[5];
        memcpy(type, png + pos + 4, 4);
        type[4] = 0;

        chunk_count++;

        if (strcmp(type, CHUNK_TYPE) == 0) {
            unsigned char *data = png + pos + 8;
            
            size_t dec_len;
            unsigned char *dec = decrypt_payload(data, len, pass, &dec_len);
            
            printf("[*] Extracting files to: %s\n", out_dir ? out_dir : "current directory");
            unpack_file(dec, dec_len, out_dir);
            
            free(dec);
            found = 1;
            break;
        }

        pos += 12 + len;
    }

    free(png);

    if (!found) {
        fprintf(stderr, "[-] No hidden payload found in '%s' (scanned %d chunks)\n", stego, chunk_count);
        fprintf(stderr, "[-] The file might not contain steganographic data or uses a different chunk type\n");
        exit(1);
    }
}
