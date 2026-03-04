#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <sys/stat.h>
#include "core/util.h"
#include "core/png.h"

#define MAX_PASSPHRASE_LEN 256

volatile sig_atomic_t sigint_received = 0;

void sigint_handler(int sig) {
    (void)sig;
    sigint_received = 1;
}

void print_usage(const char *prog_name) {
    printf("Arconda: Advanced PNG-Based File Steganography\n");
    printf("Usage:\n");
    printf("  Encode: %s -e -i <input.png> -o <output.png> -s <secret_file>\n", prog_name);
    printf("  Decode: %s -d -i <stego.png> [-o <output_dir>]\n", prog_name);
    printf("\nOptions:\n");
    printf("  -e, --encode              Encode mode (hide file in PNG)\n");
    printf("  -d, --decode              Decode mode (extract hidden file)\n");
    printf("  -i, --input FILE          Input PNG file\n");
    printf("  -o, --output FILE/DIR     Output PNG file (encode) or directory (decode - optional)\n");
    printf("  -s, --secret FILE         File to hide (encode mode only)\n");
    printf("  -h, --help                Show this help message\n");
}

int file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

int directory_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        return 1;
    }
    return 0;
}

char* get_passphrase(const char *prompt) {
    struct termios oldt, newt;
    char *passphrase = malloc(MAX_PASSPHRASE_LEN);
    if (!passphrase) {
        fprintf(stderr, "[-] Memory allocation failed\n");
        return NULL;
    }

    int i = 0;
    int ch;

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    newt.c_lflag &= ~(ECHO | ICANON | ECHOE | ECHOK | ECHONL);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    printf("%s", prompt);
    fflush(stdout);

    while (i < MAX_PASSPHRASE_LEN - 1) {
        if (sigint_received) {
            printf("\n");
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            free(passphrase);
            return NULL;
        }

        ch = getchar();
        
        if (ch == EOF) {
            break;
        }
        
        if (ch == '\n' || ch == '\r') {
            break;
        } else if (ch == 127 || ch == '\b') {
            if (i > 0) {
                i--;
                printf("\b \b");
                fflush(stdout);
            }
        } else if (ch >= 32 && ch <= 126) {
            passphrase[i++] = ch;
            printf("*");
            fflush(stdout);
        }
    }

    passphrase[i] = '\0';
    printf("\n");
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    signal(SIGINT, SIG_DFL);

    return passphrase;
}

char* get_passphrase_with_confirm(const char *prompt) {
    char *pass1, *pass2;
    
    pass1 = get_passphrase(prompt);
    if (!pass1) return NULL;
    
    if (strlen(pass1) == 0) {
        printf("[-] Passphrase cannot be empty!\n");
        free(pass1);
        return NULL;
    }
    
    pass2 = get_passphrase("[+] Confirm passphrase: ");
    if (!pass2) {
        free(pass1);
        return NULL;
    }
    
    if (strcmp(pass1, pass2) != 0) {
        printf("[-] Passphrases do not match!\n");
        free(pass1);
        free(pass2);
        return NULL;
    }
    
    free(pass2);
    return pass1;
}

int main(int argc, char *argv[]) {
    int encode_mode = 0;
    int decode_mode = 0;
    char *input = NULL;
    char *output = NULL;
    char *secret = NULL;
    char *passphrase = NULL;

    static struct option long_options[] = {
        {"encode",     no_argument,       0, 'e'},
        {"decode",     no_argument,       0, 'd'},
        {"input",      required_argument, 0, 'i'},
        {"output",     required_argument, 0, 'o'},
        {"secret",     required_argument, 0, 's'},
        {"help",       no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "edi:o:s:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'e':
                encode_mode = 1;
                break;
            case 'd':
                decode_mode = 1;
                break;
            case 'i':
                input = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            case 's':
                secret = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if ((encode_mode && decode_mode) || (!encode_mode && !decode_mode)) {
        fprintf(stderr, "[-] Must specify either encode (-e) or decode (-d) mode\n");
        print_usage(argv[0]);
        return 1;
    }

    if (!input) {
        fprintf(stderr, "[-] Input file (-i) is required\n");
        print_usage(argv[0]);
        return 1;
    }

    if (!file_exists(input)) {
        fprintf(stderr, "[-] Input file does not exist: '%s'\n", input);
        return 1;
    }

    if (encode_mode) {
        if (!output) {
            fprintf(stderr, "[-] Encode mode requires output file (-o)\n");
            print_usage(argv[0]);
            return 1;
        }
        if (!secret) {
            fprintf(stderr, "[-] Encode mode requires secret file (-s)\n");
            print_usage(argv[0]);
            return 1;
        }
        
        if (!file_exists(secret)) {
            fprintf(stderr, "[-] Secret file does not exist: '%s'\n", secret);
            return 1;
        }
        
        char *output_copy = strdup(output);
        if (!output_copy) {
            fprintf(stderr, "[-] Memory allocation failed\n");
            return 1;
        }
        
        char *output_dir = dirname(output_copy);
        if (strcmp(output_dir, ".") != 0 && strcmp(output_dir, "/") != 0 && !directory_exists(output_dir)) {
            fprintf(stderr, "[-] Output directory does not exist: '%s'\n", output_dir);
            free(output_copy);
            return 1;
        }
        free(output_copy);

        passphrase = get_passphrase_with_confirm("[+] Enter passphrase: ");
        if (!passphrase) {
            fprintf(stderr, "[-] Failed to get passphrase\n");
            return 1;
        }
        
        encode_png(input, output, secret, passphrase);
        
    } else {
        if (output) {
            struct stat st;
            if (stat(output, &st) == -1) {
                printf("[*] Output directory will be created: %s\n", output);
            } else if (!S_ISDIR(st.st_mode)) {
                fprintf(stderr, "[-] Output path exists but is not a directory: '%s'\n", output);
                return 1;
            }
            printf("[*] Output directory: %s\n", output);
        } else {
            printf("[*] Output directory: current directory (not specified)\n");
        }
        
        passphrase = get_passphrase("[+] Enter passphrase: ");
        if (!passphrase) {
            fprintf(stderr, "[-] Failed to get passphrase\n");
            return 1;
        }
        
        if (strlen(passphrase) == 0) {
            printf("[-] Passphrase cannot be empty!\n");
            free(passphrase);
            return 1;
        }
        decode_png(input, passphrase, output);
    }

    if (passphrase) {
        memset(passphrase, 0, strlen(passphrase));
        free(passphrase);
    }

    return 0;
}
