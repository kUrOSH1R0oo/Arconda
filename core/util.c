#include "util.h"

void die(const char *msg) {
    fprintf(stderr, "[-] %s\n", msg);
    exit(1);
}

void die_with_file(const char *msg, const char *filename) {
    fprintf(stderr, "[-] %s: '%s'\n", msg, filename);
    exit(1);
}

void read_file(const char *path, unsigned char **buf, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (!f) die_with_file("Cannot open input file", path);

    fseek(f, 0, SEEK_END);
    *len = ftell(f);
    rewind(f);

    *buf = malloc(*len);
    if (!*buf) {
        fclose(f);
        die("Memory allocation failed");
    }

    if (fread(*buf, 1, *len, f) != *len) {
        fclose(f);
        free(*buf);
        die_with_file("Error reading file", path);
    }
    fclose(f);
}

void write_file(const char *path, unsigned char *buf, size_t len) {
    char *path_copy = strdup(path);
    if (!path_copy) die("Memory allocation failed");
    
    char *dir = dirname(path_copy);
    
    if (strcmp(dir, ".") != 0 && strcmp(dir, "/") != 0) {
        struct stat st;
        if (stat(dir, &st) == -1) {
            free(path_copy);
            die_with_file("Output directory does not exist", dir);
        }
        if (!S_ISDIR(st.st_mode)) {
            free(path_copy);
            die_with_file("Output path exists but is not a directory", dir);
        }
    }
    free(path_copy);

    FILE *f = fopen(path, "wb");
    if (!f) die_with_file("Cannot create output file", path);

    if (fwrite(buf, 1, len, f) != len) {
        fclose(f);
        die_with_file("Error writing to file", path);
    }
    fclose(f);
}