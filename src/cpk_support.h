#ifndef CETTAPACK_SUPPORT_H
#define CETTAPACK_SUPPORT_H
#include <stdbool.h>
#include <stddef.h>

typedef enum { CPK_JSON_NULL, CPK_JSON_BOOL, CPK_JSON_NUMBER, CPK_JSON_STRING,
               CPK_JSON_ARRAY, CPK_JSON_OBJECT } CpkJsonKind;
typedef struct CpkJson {
    CpkJsonKind kind;
    double num;
    char *str;
    size_t n;
    char **keys;              /* objects only */
    struct CpkJson **items;
} CpkJson;

CpkJson *cpk_json_parse(const char *text);
void cpk_json_free(CpkJson *j);
const CpkJson *cpk_json_get(const CpkJson *j, const char *dotted);
const char *cpk_json_str(const CpkJson *j, const char *dotted);

char *cpk_read_file(const char *path, size_t *len_out);
void cpk_sha256_hex(const void *data, size_t len, char out[65]);
bool cpk_tree_digest(const char *root, char out[65]);
#endif
