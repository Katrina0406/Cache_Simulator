/**
 * @Name Yuqiao Hu
 * @AndrewID yuqiaohu
 *
 * File Introduction:
 * This is a cache simulator which reads in flags from terminal as parameters
 * of the cache (s, E, b in any order), filename from terminal as the trace file
 * and output cache behaviors (resulting memory info).
 * [sample terminal message]:
 * ./csim -s 0 -E 1 -b 0 -t traces/csim/wide.trace
 *
 * Design:
 * I used 3 structs to implement cache.
 * block_t struct contains valid bit, bytes, tag, visit times, and dirty bit
 * set_t struct contains lines per set (represented as block_t **)
 * cache_t struct contains total sets (represented as set_t **)
 * I choose this design because it's very clear and mimics the actual structure
 * of a cache very well
 *
 * some helper functions include hex2bin (used for converting hex address to
 * binary form to better split the block into t, s, b sections) and bin2deci
 * (used for getting the decimal number of the target set from binary set
 * number)
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cachelab.h"
#include <getopt.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

// get s, E, b, filename from terminal flags using getopt
int *getParams(int argc, char *argv[], char **filename) {

    int option;
    int s, E, b;
    while ((option = getopt(argc, argv, "s:v1:E:v2:b:v3:t:v4")) != -1) {
        switch (option) {
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            *filename = optarg;
            break;
        }
    }

    int *result = malloc(sizeof(int) * 3);
    result[0] = s;
    result[1] = E;
    result[2] = b;

    return result;
}

// convert hexadecimal to binary
// used for converting hex addr to bin form
char *hex2bin(char *hexa) {
    int count = 0;
    char *result = malloc(1);
    strcpy(result, "");
    while (hexa[count]) {
        switch (hexa[count]) {
        case '0':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "0000");
            break;
        case '1':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "0001");
            break;
        case '2':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "0010");
            break;
        case '3':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "0011");
            break;
        case '4':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "0100");
            break;
        case '5':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "0101");
            break;
        case '6':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "0110");
            break;
        case '7':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "0111");
            break;
        case '8':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "1000");
            break;
        case '9':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "1001");
            break;
        case 'A':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "1010");
            break;
        case 'B':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "1011");
            break;
        case 'C':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "1100");
            break;
        case 'D':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "1101");
            break;
        case 'E':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "1110");
            break;
        case 'F':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "1111");
            break;
        case 'a':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "1010");
            break;
        case 'b':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "1011");
            break;
        case 'c':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "1100");
            break;
        case 'd':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "1101");
            break;
        case 'e':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "1110");
            break;
        case 'f':
            result = (char *)realloc(result, strlen(result) + 5);
            strcat(result, "1111");
            break;
        }
        count++;
    }
    return result;
}

// slice part of a string
void slice_str(const char *str, char *buffer, int start, int end) {
    int j = 0;
    for (int i = start; i < end; ++i) {
        buffer[j++] = str[i];
    }
    buffer[j] = 0;
}

// convert binary to decimal
// used for converting binary set number to decimal one
int bin2deci(int binary) {
    int i = 0;
    int result = 0;
    while (binary > 0) {
        int last = binary % 10;
        result += last * pow(2, i);
        binary /= 10;
        i++;
    }
    return result;
}

// fill "times" 0s ahead of string tag
// used for making the binary addr 64 bits because hex2bin only converts
// existing hexa number, so there are no 0s ahead to make it 64 bits
char *fill_zero(char *tag, int times) {
    unsigned long len = strlen(tag);
    char *result_tag = malloc(len + 1);
    strcpy(result_tag, tag);
    char *tmp = malloc(2);
    strcpy(tmp, "0");
    for (int i = 0; i < times; i++) {
        unsigned long len_r = strlen(result_tag);
        tmp = (char *)realloc(tmp, len_r + 2);
        strcat(tmp, result_tag);
        unsigned long len_l = strlen(tmp);
        result_tag = (char *)realloc(result_tag, len_l + 1);
        strcpy(result_tag, tmp);
        tmp = (char *)realloc(tmp, 2);
        strcpy(tmp, "0");
    }
    free(tmp);
    return result_tag;
}

// get the number of the set the addr leads to
int set_number(char *addr, int s, int b) {
    int len = (int)(long)strlen(addr);
    int times = 64 - len;
    char *actual_addr = fill_zero(addr, times);
    char buffer[strlen(actual_addr)];
    slice_str(actual_addr, buffer, 64 - b - s, 64 - b);
    int binary = atoi(buffer);
    int setn = bin2deci(binary);
    free(actual_addr);
    // printf("set: %d\n", setn);
    return setn;
}

// get the number of the tag the addr has
char *tag_number(char *addr, int s, int b) {
    int len = (int)(long)strlen(addr);
    char buffer[65];
    slice_str(addr, buffer, 0, len - b - s);
    char *tag = malloc(65);
    int len_tag = (int)(long)strlen(buffer);
    int times = 64 - (s + b + len_tag);
    strcpy(tag, buffer);
    char *result_tag = fill_zero(tag, times);
    free(tag);
    // printf("tag: %s\n", result_tag);
    return result_tag;
}

void do_operations(bool isLoad, int *v, char **t, int *vis, int *d, int E,
                   int b, char *tag, int *total_visits, csim_stats_t *stats) {
    int i = 0;
    bool has_space = false;
    bool has_empty = false;
    while (i < E) {
        // match existing block
        // printf("%s\n", t[i]);
        if (v[i] == 1 && strcmp(t[i], tag) == 0) {
            *total_visits += 1;
            vis[i] = *total_visits;
            stats->hits += 1;
            has_space = true;
            if (!isLoad) {
                if (d[i] == 0) {
                    d[i] = 1;
                    stats->dirty_bytes += pow(2, b);
                }
            }
            // printf("load/store hit %d\n", i);
            break;
        }
        // load/store bytes into empty line
        if (v[i] == 0) {
            has_empty = true;
        }
        i++;
    }
    if (!has_space) {
        if (has_empty) {
            // load/store bytes into empty line
            int i = 0;
            while (i < E) {
                if (v[i] == 0) {
                    v[i] = 1;
                    strcpy(t[i], tag);
                    *total_visits += 1;
                    vis[i] = *total_visits;
                    stats->misses += 1;
                    if (!isLoad) {
                        d[i] = 1;
                        stats->dirty_bytes += pow(2, b);
                    }
                    // printf("load/store miss %d\n", i);
                    // printf("change tag to %s\n", t[i]);
                    break;
                }
                i++;
            }
        } else {
            // choose a line to evict and load bytes in (miss eviction)
            // find the LRU line
            int i = 1;
            int least_visit = vis[0];
            int least_i = 0;
            while (i < E) {
                if (vis[i] < least_visit) {
                    least_visit = vis[i];
                    least_i = i;
                }
                i++;
            }
            // load/store bytes into this block (LRU)
            if (isLoad) {
                if (d[least_i] == 1) {
                    // if last op is S, evict dirty bytes
                    d[least_i] = 0;
                    stats->dirty_bytes -= pow(2, b);
                    stats->dirty_evictions += pow(2, b);
                    // printf("load miss evict %d\n", i);
                }
            } else {
                if (d[least_i] == 0) {
                    // if last op is L, add dirty bytes
                    d[least_i] = 1;
                    stats->dirty_bytes += pow(2, b);
                } else {
                    stats->dirty_evictions += pow(2, b);
                    // printf("store miss evict %d\n", i);
                }
            }
            // normal eviction
            stats->evictions += 1;
            stats->misses += 1;
            *total_visits += 1;
            vis[least_i] = *total_visits;
            // printf("visits: %d\n", vis[least_i]);
            strcpy(t[least_i], tag);
            // printf("load/store miss evict %d\n", least_i);
        }
    }
}

int main(int argc, char *argv[]) {

    char **filename = malloc(sizeof(char *));
    int *params = getParams(argc, argv, filename);
    int s = params[0];
    int E = params[1];
    int b = params[2];

    FILE *fp = fopen(*filename, "r");
    if (fp == NULL) {
        perror("Unable to open file!");
        exit(1);
    }

    csim_stats_t *stats = calloc(1, sizeof(csim_stats_t));

    int S = (int)pow(2, s);

    int i;
    // create cache
    int **valid_bit = calloc((unsigned long)(long)(S * E), sizeof(int *));
    for (i = 0; i < S; i++) {
        valid_bit[i] = calloc((unsigned long)(long)E, sizeof(int *));
    }
    char ***tagt = calloc((unsigned long)(long)(S), sizeof(char **));
    for (i = 0; i < S; i++) {
        tagt[i] = calloc((unsigned long)(long)E, sizeof(char *));
        for (int j = 0; j < E; j++) {
            tagt[i][j] = calloc(65, sizeof(char));
        }
    }
    int **visit = calloc((unsigned long)(long)(S * E), sizeof(int *));
    for (i = 0; i < S; i++) {
        visit[i] = calloc((unsigned long)(long)E, sizeof(int *));
    }
    int **dirty_bit = calloc((unsigned long)(long)(S * E), sizeof(int *));
    for (i = 0; i < S; i++) {
        dirty_bit[i] = calloc((unsigned long)(long)E, sizeof(int *));
    }

    int *total_visits = calloc(1, sizeof(int));

    char line[128];
    // read each line
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *op = strtok(line, " ");
        char *addr = strtok(NULL, ",");
        char *bin_addr = hex2bin(addr);
        int setn = set_number(bin_addr, s, b);
        char *tag = tag_number(bin_addr, s, b);

        int *v = valid_bit[setn];
        char **t = tagt[setn];
        int *vis = visit[setn];
        int *d = dirty_bit[setn];

        if (strcmp(op, "L") == 0) {
            // Load
            do_operations(true, v, t, vis, d, E, b, tag, total_visits, stats);
        } else {
            // Store
            do_operations(false, v, t, vis, d, E, b, tag, total_visits, stats);
        }
        free(bin_addr);
        free(tag);
    }

    fclose(fp);

    printSummary(stats);

    // free memory allocated
    free(filename);
    free(stats);
    free(params);
    free(total_visits);
    for (i = 0; i < S; i++) {
        free(valid_bit[i]);
    }
    free(valid_bit);
    for (i = 0; i < S; i++) {
        for (int j = 0; j < E; j++) {
            free(tagt[i][j]);
        }
        free(tagt[i]);
    }
    free(tagt);
    for (i = 0; i < S; i++) {
        free(visit[i]);
    }
    free(visit);
    for (i = 0; i < S; i++) {
        free(dirty_bit[i]);
    }
    free(dirty_bit);

    return 0;
}
