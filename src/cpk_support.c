/* Minimal JSON reader (manifest) and CettaPack digests. */
#define _XOPEN_SOURCE 700
#include "cpk_support.h"

#include "native_sha256.h"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* ---- JSON ---------------------------------------------------------------- */

static void skip_ws(const char **p) { while (**p && isspace((unsigned char)**p)) (*p)++; }

static void json_free_node(CpkJson *j) {
    if (!j) return;
    for (size_t i = 0; i < j->n; i++) { free(j->keys ? j->keys[i] : NULL); json_free_node(j->items[i]); }
    free(j->keys); free(j->items); free(j->str); free(j);
}
void cpk_json_free(CpkJson *j) { json_free_node(j); }

static char *parse_string(const char **p) {
    if (**p != '"') return NULL;
    (*p)++;
    size_t cap = 32, len = 0;
    char *s = malloc(cap);
    while (s && **p && **p != '"') {
        char c = **p;
        if (c == '\\') {
            (*p)++;
            c = **p;
            if (c == 'n') c = '\n'; else if (c == 't') c = '\t';
            else if (c == 'u') { free(s); return NULL; }  /* not needed by manifests */
        }
        if (len + 2 > cap) { cap *= 2; char *t = realloc(s, cap); if (!t) { free(s); return NULL; } s = t; }
        s[len++] = c;
        (*p)++;
    }
    if (!s || **p != '"') { free(s); return NULL; }
    (*p)++;
    s[len] = 0;
    return s;
}

static CpkJson *parse_value(const char **p, int depth);

static bool push_item(CpkJson *j, char *key, CpkJson *v) {
    char **k = realloc(j->keys, (j->n + 1) * sizeof(char *));
    if (!k) return false;
    j->keys = k;
    CpkJson **it = realloc(j->items, (j->n + 1) * sizeof(CpkJson *));
    if (!it) return false;
    j->items = it;
    j->keys[j->n] = key; j->items[j->n] = v; j->n++;
    return true;
}

static CpkJson *parse_value(const char **p, int depth) {
    if (depth > 32) return NULL;
    skip_ws(p);
    CpkJson *j = calloc(1, sizeof(*j));
    if (!j) return NULL;
    char c = **p;
    if (c == '{' || c == '[') {
        j->kind = c == '{' ? CPK_JSON_OBJECT : CPK_JSON_ARRAY;
        char close = c == '{' ? '}' : ']';
        (*p)++;
        skip_ws(p);
        if (**p == close) { (*p)++; return j; }
        for (;;) {
            char *key = NULL;
            skip_ws(p);
            if (j->kind == CPK_JSON_OBJECT) {
                key = parse_string(p);
                skip_ws(p);
                if (!key || **p != ':') { free(key); json_free_node(j); return NULL; }
                (*p)++;
            }
            CpkJson *v = parse_value(p, depth + 1);
            if (!v || !push_item(j, key, v)) { free(key); json_free_node(v); json_free_node(j); return NULL; }
            skip_ws(p);
            if (**p == ',') { (*p)++; continue; }
            if (**p == close) { (*p)++; return j; }
            json_free_node(j); return NULL;
        }
    }
    if (c == '"') { j->kind = CPK_JSON_STRING; j->str = parse_string(p); if (!j->str) { free(j); return NULL; } return j; }
    if (!strncmp(*p, "true", 4)) { j->kind = CPK_JSON_BOOL; j->num = 1; *p += 4; return j; }
    if (!strncmp(*p, "false", 5)) { j->kind = CPK_JSON_BOOL; j->num = 0; *p += 5; return j; }
    if (!strncmp(*p, "null", 4)) { j->kind = CPK_JSON_NULL; *p += 4; return j; }
    char *end = NULL;
    j->num = strtod(*p, &end);
    if (end == *p) { free(j); return NULL; }
    j->kind = CPK_JSON_NUMBER; *p = end;
    return j;
}

CpkJson *cpk_json_parse(const char *text) {
    const char *p = text;
    CpkJson *j = parse_value(&p, 0);
    if (!j) return NULL;
    skip_ws(&p);
    if (*p) { json_free_node(j); return NULL; }
    return j;
}

const CpkJson *cpk_json_get(const CpkJson *j, const char *dotted) {
    char buf[256];
    snprintf(buf, sizeof buf, "%s", dotted);
    char *save = NULL;
    for (char *tok = strtok_r(buf, ".", &save); tok && j; tok = strtok_r(NULL, ".", &save)) {
        if (j->kind != CPK_JSON_OBJECT) return NULL;
        const CpkJson *next = NULL;
        for (size_t i = 0; i < j->n; i++) if (!strcmp(j->keys[i], tok)) { next = j->items[i]; break; }
        j = next;
    }
    return j;
}

const char *cpk_json_str(const CpkJson *j, const char *dotted) {
    const CpkJson *v = cpk_json_get(j, dotted);
    return v && v->kind == CPK_JSON_STRING ? v->str : NULL;
}

/* ---- files and digests -------------------------------------------------- */

char *cpk_read_file(const char *path, size_t *len_out) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = n >= 0 ? malloc((size_t)n + 1) : NULL;
    if (buf && fread(buf, 1, (size_t)n, f) != (size_t)n) { free(buf); buf = NULL; }
    fclose(f);
    if (buf) { buf[n] = 0; if (len_out) *len_out = (size_t)n; }
    return buf;
}

void cpk_sha256_hex(const void *data, size_t len, char out[65]) {
    cetta_native_sha256_hex((const uint8_t *)data, len, out);
}

static int cmp_str(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }

/* Collect regular files below root (relative paths, sorted). */
static bool collect(const char *root, const char *rel, char ***out, size_t *n, size_t *cap) {
    char path[4096];
    snprintf(path, sizeof path, "%s%s%s", root, rel[0] ? "/" : "", rel);
    DIR *d = opendir(path);
    if (!d) return false;
    struct dirent *e;
    bool ok = true;
    while (ok && (e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        char sub[4096], full[8192];
        snprintf(sub, sizeof sub, "%s%s%s", rel, rel[0] ? "/" : "", e->d_name);
        snprintf(full, sizeof full, "%s/%s", root, sub);
        struct stat st;
        if (stat(full, &st)) { ok = false; break; }
        if (S_ISDIR(st.st_mode)) { ok = collect(root, sub, out, n, cap); continue; }
        if (!S_ISREG(st.st_mode)) continue;
        if (*n == *cap) { *cap = *cap ? *cap * 2 : 64; char **t = realloc(*out, *cap * sizeof(char *)); if (!t) { ok = false; break; } *out = t; }
        (*out)[(*n)++] = strdup(sub);
    }
    closedir(d);
    return ok;
}

/* cpk-tree-v1: sha256 over lines "<relpath>\0<sha256(content)>\n", sorted by relpath. */
bool cpk_tree_digest(const char *root, char out[65]) {
    char **files = NULL; size_t n = 0, cap = 0;
    if (!collect(root, "", &files, &n, &cap)) { for (size_t i = 0; i < n; i++) free(files[i]); free(files); return false; }
    qsort(files, n, sizeof(char *), cmp_str);
    CettaNativeSha256 sha;
    cetta_native_sha256_init(&sha);
    cetta_native_sha256_update(&sha, (const uint8_t *)"cpk-tree-v1\n", 12);
    bool ok = true;
    for (size_t i = 0; i < n; i++) {
        char full[8192];
        snprintf(full, sizeof full, "%s/%s", root, files[i]);
        size_t len = 0;
        char *data = cpk_read_file(full, &len);
        if (!data) { ok = false; break; }
        char h[65];
        cpk_sha256_hex(data, len, h);
        free(data);
        cetta_native_sha256_update(&sha, (const uint8_t *)files[i], strlen(files[i]) + 1);
        cetta_native_sha256_update(&sha, (const uint8_t *)h, 64);
        cetta_native_sha256_update(&sha, (const uint8_t *)"\n", 1);
    }
    for (size_t i = 0; i < n; i++) free(files[i]);
    free(files);
    if (ok) cetta_native_sha256_finish_hex(&sha, out);
    return ok;
}
