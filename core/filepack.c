#include "filepack.h"
#include "util.h"

unsigned char *pack_file(const char *path, size_t *out_len) {
    unsigned char *data;
    size_t len;
    
    read_file(path, &data, &len);

    char *path_copy = strdup(path);
    if (!path_copy) {
        free(data);
        die("Memory allocation failed");
    }
    
    char *filename = basename(path_copy);
    uint16_t name_len = strlen(filename);
    
    *out_len = 2 + name_len + 8 + len;
    unsigned char *buf = malloc(*out_len);
    if (!buf) {
        free(path_copy);
        free(data);
        die("Memory allocation failed for packed data");
    }

    unsigned char *p = buf;
    uint16_t be16 = htons(name_len);
    memcpy(p, &be16, 2);
    p += 2;

    memcpy(p, filename, name_len);
    p += name_len;

    uint64_t be64 = htobe64(len);
    memcpy(p, &be64, 8);
    p += 8;

    memcpy(p, data, len);

    free(path_copy);
    free(data);
    return buf;
}

void unpack_file(unsigned char *blob, size_t blob_len, const char *out_dir) {
    size_t pos = 0;
    if (blob_len < 2) die("Extracted data is corrupted (too small)");

    uint16_t name_len;
    memcpy(&name_len, blob, 2);
    name_len = ntohs(name_len);
    pos += 2;

    if (pos + name_len + 8 > blob_len) die("Extracted data is corrupted (filename length mismatch)");

    char filename[256];
    if (name_len >= sizeof(filename)) die("Extracted filename is too long");
    
    memcpy(filename, blob + pos, name_len);
    filename[name_len] = 0;
    pos += name_len;

    uint64_t fsize;
    memcpy(&fsize, blob + pos, 8);
    fsize = be64toh(fsize);
    pos += 8;

    if (pos + fsize > blob_len) die("Extracted data is corrupted (file size mismatch)");

    char out_path[512];
    
    if (out_dir && strlen(out_dir) > 0) {
        struct stat st;
        if (stat(out_dir, &st) == -1) {
            fprintf(stderr, "[!] Output directory does not exist, creating: %s\n", out_dir);
            #ifdef _WIN32
                if (mkdir(out_dir) != 0)
            #else
                if (mkdir(out_dir, 0700) != 0)
            #endif
                {
                    die_with_file("Failed to create output directory", out_dir);
                }
        } else if (!S_ISDIR(st.st_mode)) {
            die_with_file("Output path exists but is not a directory", out_dir);
        }
        snprintf(out_path, sizeof(out_path), "%s/%s", out_dir, filename);
    } else {
        snprintf(out_path, sizeof(out_path), "%s", filename);
    }
    
    FILE *test = fopen(out_path, "rb");
    if (test) {
        fclose(test);
        fprintf(stderr, "[!] File already exists, overwriting: %s\n", out_path);
    }
    
    write_file(out_path, blob + pos, fsize);
    printf("[+] Successfully extracted: %s\n", out_path);
}