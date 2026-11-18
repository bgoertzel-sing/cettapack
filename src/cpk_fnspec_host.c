/* cettapack-fnspec: whole-function specialisation host (t-2026-11-18-cettapack-fnspec).
 *
 *   cettapack-fnspec list
 *   cettapack-fnspec defs --site S
 *   cettapack-fnspec bench --site S --kb FILE --queries FILE --mode MODE [--repeat N]
 *   cettapack-fnspec difftest --site S --cases DIR [--kb FILE --queries FILE]
 *
 * Modes (every query atom built before the timed loop; one untimed warm-up call):
 *   fn-generic   (f [S] args...)              CeTTa evaluates the function
 *   fn-hosted    (f-h [S] args...)            the same function with its match replaced by
 *                                             cpk-hosted:f (CeTTa's conjunction matcher behind an
 *                                             embedding callable, as the probes' hosted-generic);
 *                                             the rest of the body evaluated by CeTTa
 *   fn-spec      (cpk-fnspec:f [S] args...)   the compiled function: guards, CeTTa's matcher,
 *                                             condition and template in C, CeTTa's conventions
 *   fn-fallback  the same callable with its guard forced INVALID: the residual
 *
 * A query line is one argument atom, or a tuple of the non-space arguments.
 */
#define _POSIX_C_SOURCE 200809L
#include "cpk_fnspec.h"
#include "cpk_support.h"
#include <time.h>
static uint64_t cpk_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
}
#include "space_match_backend.h"

#include <dirent.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

static void die(const char *f, ...) {
    va_list ap; va_start(ap, f);
    fprintf(stderr, "cettapack-fnspec: "); vfprintf(stderr, f, ap); fprintf(stderr, "\n");
    va_end(ap);
    exit(2);
}

static cetta_engine_t *engine_new(void) {
    cetta_engine_config_t cfg = {.size = sizeof cfg, .abi_version = CETTA_EMBED_ABI_VERSION,
                                 .language = "he", .resource_root = CPK_CETTA_ROOT};
    cetta_engine_t *e = NULL;
    cetta_status_t st = cetta_embed_engine_new(&cfg, &e);
    if (st != CETTA_OK) die("engine: %s", cetta_embed_status_name(st));
    return e;
}
static cetta_atom_t *parse1(cetta_engine_t *e, const char *t) {
    cetta_atom_t *a = NULL;
    cetta_status_t st = cetta_embed_parse(e, t, strlen(t), &a);
    if (st != CETTA_OK) die("parse '%s': %s", t, cetta_embed_status_name(st));
    return a;
}
static cetta_atom_t *sym(cetta_engine_t *e, const char *s) {
    cetta_atom_t *a = NULL;
    if (cetta_embed_atom_symbol(e, s, &a) != CETTA_OK) die("symbol");
    return a;
}
static char *fmt_atom(const cetta_atom_t *a) {
    size_t need = 0;
    cetta_embed_atom_format(a, NULL, 0, &need);
    char *s = malloc(need + 1);
    if (!s || cetta_embed_atom_format(a, s, need + 1, &need) != CETTA_OK) die("format");
    return s;
}
static void run_text(cetta_engine_t *e, const char *t) {
    cetta_result_t *r = NULL;
    cetta_status_t st = cetta_embed_run(e, t, strlen(t), NULL, &r);
    if (st != CETTA_OK || cetta_embed_result_status(r) != CETTA_OK) die("run: %s\n%s", cetta_embed_status_name(st), t);
    cetta_embed_result_release(r);
}

/* ---- bags ------------------------------------------------------------------------ */

typedef struct { char **items; size_t n; } Bag;
static int cmp_s(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }
static void bag_free(Bag *b) { for (size_t i = 0; i < b->n; i++) free(b->items[i]); free(b->items); b->items = NULL; b->n = 0; }
static bool g_eval_error;
/* results as text, variable names normalised per answer; sorted (a bag) */
static char *answer_text(const cetta_atom_t *h) {
    char *s = fmt_atom(h);
    /* CeTTa prints fresh variables with a #suffix; normalise by first occurrence */
    char *out = malloc(strlen(s) * 2 + 16), *o = out;
    char names[64][64]; int nn = 0;
    for (char *p = s; *p;) {
        if (!strncmp(p, "<space 0x", 9)) {   /* a space atom prints its address: engine-specific */
            char *q = strchr(p, '>');
            if (q) { o += sprintf(o, "<space>"); p = q + 1; continue; }
        }
        if (*p == '$') {
            char *q = p + 1;
            while (*q && *q != ' ' && *q != ')' && *q != '(') q++;
            size_t l = (size_t)(q - p);
            char nm[64]; snprintf(nm, sizeof nm, "%.*s", (int)(l < 63 ? l : 63), p);
            int k = -1;
            for (int i = 0; i < nn; i++) if (!strcmp(names[i], nm)) k = i;
            if (k < 0 && nn < 64) { strcpy(names[nn], nm); k = nn++; }
            o += sprintf(o, "$_%d", k);
            p = q;
        } else *o++ = *p++;
    }
    *o = 0;
    free(s);
    return out;
}
static Bag eval_bag(cetta_engine_t *e, cetta_atom_t *x) {
    cetta_result_t *r = NULL;
    cetta_status_t st = cetta_embed_eval(e, x, NULL, &r);
    if (st == CETTA_OK && cetta_embed_result_status(r) != CETTA_OK) st = cetta_embed_result_status(r);
    if (st != CETTA_OK) {
        g_eval_error = true;
        cetta_embed_result_release(r);
        Bag b = {calloc(1, sizeof(char *)), 1};
        b.items[0] = strdup("<eval error>");
        return b;
    }
    size_t n = cetta_embed_result_count(r);
    Bag b = {calloc(n + 1, sizeof(char *)), n};
    for (size_t i = 0; i < n; i++) {
        cetta_atom_t *h = NULL;
        cetta_embed_result_get(r, i, &h);
        b.items[i] = answer_text(h);
        cetta_embed_atom_release(h);
    }
    cetta_embed_result_release(r);
    qsort(b.items, b.n, sizeof(char *), cmp_s);
    return b;
}
static bool bag_eq(const Bag *a, const Bag *b) {
    if (a->n != b->n) return false;
    for (size_t i = 0; i < a->n; i++) if (strcmp(a->items[i], b->items[i])) return false;
    return true;
}
static void show_bag(const char *label, const Bag *b) {
    fprintf(stderr, "    %s (%zu):", label, b->n);
    for (size_t i = 0; i < b->n && i < 6; i++) fprintf(stderr, " %.300s", b->items[i]);
    fprintf(stderr, "%s\n", b->n > 6 ? " ..." : "");
}

/* ---- environments ------------------------------------------------------------------ */

typedef struct { const char *name; bool fallback; } Mode;
static const Mode MODES[] = {{"fn-generic", false}, {"fn-hosted", false}, {"fn-spec", false}, {"fn-fallback", true}};
static const Mode *find_mode(const char *n) {
    for (size_t i = 0; i < sizeof MODES / sizeof *MODES; i++) if (!strcmp(MODES[i].name, n)) return &MODES[i];
    die("unknown mode %s", n);
    return NULL;
}

typedef struct { cetta_engine_t *e; FsRt *rt; cetta_atom_t *space; const FsPlan *p; const Mode *m; } Env;

static void add_text(cetta_engine_t *e, cetta_atom_t *space, const char *text) {
    cetta_space_t *s = NULL;
    if (cetta_embed_space_from_atom(e, space, &s) != CETTA_OK) die("space handle");
    for (const char *p = text; *p;) {
        const char *nl = strchr(p, '\n');
        size_t n = nl ? (size_t)(nl - p) : strlen(p);
        if (n > 0 && p[0] != ';') {
            char *line = strndup(p, n);
            cetta_atom_t *a = parse1(e, line);
            if (cetta_embed_space_add_atom(s, a) != CETTA_OK) die("add %s", line);
            cetta_embed_atom_release(a);
            free(line);
        }
        p += n + (nl ? 1 : 0);
    }
    cetta_embed_space_release(s);
}

static Env env_open(const FsPlan *p, const Mode *m, const char *kbtext) {
    Env v = {.p = p, .m = m};
    v.e = engine_new();
    if (p->space_token) {
        char t[256]; snprintf(t, sizeof t, "!(bind! %s (new-space))\n", p->space_token);
        run_text(v.e, t);
    }
    run_text(v.e, p->definitions);
    if (!strcmp(m->name, "fn-hosted")) run_text(v.e, p->hosted_def);
    if (p->space_token) {
        char t[256]; snprintf(t, sizeof t, "!(quote %s)", p->space_token);
        cetta_result_t *r = NULL;
        if (cetta_embed_run(v.e, t, strlen(t), NULL, &r) != CETTA_OK || cetta_embed_result_count(r) != 1) die("resolve %s", p->space_token);
        cetta_atom_t *q = NULL;
        cetta_embed_result_get(r, 0, &q);
        cetta_embed_atom_child(q, 1, &v.space);
        cetta_embed_atom_release(q);
        cetta_embed_result_release(r);
    } else {
        cetta_space_t *s = NULL;
        if (cetta_embed_space_new(v.e, &s) != CETTA_OK) die("new space");
        cetta_embed_space_atom(s, &v.space);
        cetta_embed_space_release(s);
    }
    if (!cetta_embed_native_space(v.space)) die("site space is not a space");
    add_text(v.e, v.space, kbtext);
    const char *err = NULL;
    v.rt = fs_open(v.e, p, m->fallback, &err);
    if (!v.rt) die("fs_open %s: %s", p->site, err);
    return v;
}
static void env_close(Env *v) {
    fs_close(v->rt);
    cetta_embed_atom_release(v->space);
    cetta_embed_engine_release(v->e);
}

static const char *mode_head(const Env *v, char *buf, size_t n) {
    const char *m = v->m->name;
    if (!strcmp(m, "fn-generic")) snprintf(buf, n, "%s", v->p->function);
    else if (!strcmp(m, "fn-hosted")) snprintf(buf, n, "%s-h", v->p->function);
    else snprintf(buf, n, "cpk-fnspec:%s", v->p->function);
    return buf;
}

/* (head [S] args...) for one query line */
static cetta_atom_t *query_atom(Env *v, const char *head, const char *line) {
    cetta_atom_t *items[18];
    size_t n = 0;
    items[n++] = sym(v->e, head);
    cetta_atom_t *q = parse1(v->e, line);
    cetta_atom_t *argv[16]; size_t na = 0;
    if (line[0] == '(') {
        size_t k = cetta_embed_atom_arity(q);
        for (size_t i = 0; i < k && na < 16; i++) cetta_embed_atom_child(q, i, &argv[na++]);
    } else {
        cetta_embed_atom_retain(q);
        argv[na++] = q;
    }
    size_t ai = 0;
    for (unsigned i = 0; i < v->p->arity; i++) {
        if ((int)i == v->p->space_arg) { cetta_embed_atom_retain(v->space); items[n++] = v->space; }
        else if (ai < na) items[n++] = argv[ai++];
        else die("query '%s' has too few arguments for %s/%u", line, v->p->function, v->p->arity);
    }
    if (ai != na) die("query '%s' has too many arguments", line);
    cetta_atom_t *x = NULL;
    if (cetta_embed_atom_expression(v->e, items, n, &x) != CETTA_OK) die("expression");
    for (size_t i = 0; i < n; i++) cetta_embed_atom_release(items[i]);
    cetta_embed_atom_release(q);
    return x;
}

static void route_json(cetta_atom_t *space, char *out, size_t len) {
    Space *s = cetta_embed_native_space(space);
    if (!s) { snprintf(out, len, "{\"error\":\"no native space\"}"); return; }
    snprintf(out, len, "{\"backend\":\"%s\",\"native_rows\":%llu,\"dense_facts\":%llu}",
             space_match_backend_kind_name(s->match_backend.kind), (unsigned long long)s->native.len,
             (unsigned long long)s->native.dense_facts);
}

static int read_lines(char *text, char **out, int max) {
    int n = 0;
    for (char *s = strtok(text, "\n"); s && n < max; s = strtok(NULL, "\n"))
        if (s[0] && s[0] != ';') out[n++] = s;
    return n;
}

static int cmp_u64(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return x < y ? -1 : x > y;
}

static void stats_json(const FsRt *rt, char *out, size_t n) {
    const FsStats *s = &rt->st;
    snprintf(out, n, "\"fn_calls\":%" PRIu64 ",\"fn_spec_calls\":%" PRIu64 ",\"fn_residual_calls\":%" PRIu64
             ",\"fn_forced\":%" PRIu64 ",\"fn_invalid\":%" PRIu64 ",\"fn_arg_rejects\":%" PRIu64 ",\"fn_row_aborts\":%" PRIu64
             ",\"fn_revalidations\":%" PRIu64 ",\"hosted_calls\":%" PRIu64 ",\"guard_valid\":%s",
             s->calls, s->spec_calls, s->residual_calls, s->forced, s->invalid, s->arg_rejects, s->row_aborts,
             s->revalidations, s->hosted_calls, rt->valid ? "true" : "false");
}

/* ---- bench ------------------------------------------------------------------------- */

/* structural check (docs/EXPERIMENT_SANITY_GATE.md): an answer that is an Error, or
 * an unevaluated call of the function, its hosted variant or a callable, is not a result */
static size_t bad_answer(const FsPlan *p, const cetta_atom_t *h) {
    if (cetta_embed_atom_kind(h) != CETTA_ATOM_EXPRESSION || cetta_embed_atom_arity(h) == 0) return 0;
    cetta_atom_t *c0 = NULL;
    if (cetta_embed_atom_child(h, 0, &c0) != CETTA_OK) return 1;
    size_t bad = 0;
    if (cetta_embed_atom_kind(c0) == CETTA_ATOM_SYMBOL) {
        char t[512]; size_t need = 0;
        if (cetta_embed_atom_get_text(c0, t, sizeof t, &need) == CETTA_OK) {
            size_t fl = strlen(p->function);
            bad = !strcmp(t, "Error") || !strncmp(t, "cpk-", 4) ||
                  (!strncmp(t, p->function, fl) && (t[fl] == 0 || !strcmp(t + fl, "-h")));
        }
    }
    cetta_embed_atom_release(c0);
    return bad;
}

static int cmd_bench(const FsPlan *p, const char *kbfile, const char *qfile, const char *mode, int repeat) {
    const Mode *m = find_mode(mode);
    char *kb = cpk_read_file(kbfile, NULL), *qs = cpk_read_file(qfile, NULL);
    if (!kb || !qs) die("read inputs");
    static char *lines[8192];
    int nq = read_lines(qs, lines, 8192);
    uint64_t t0 = cpk_now_ns();
    Env v = env_open(p, m, kb);
    uint64_t load_ns = cpk_now_ns() - t0;
    free(kb);
    char head[256];
    mode_head(&v, head, sizeof head);
    cetta_atom_t **qa = calloc((size_t)nq, sizeof(*qa));
    for (int i = 0; i < nq; i++) qa[i] = query_atom(&v, head, lines[i]);
    {
        cetta_result_t *r = NULL;
        if (cetta_embed_eval(v.e, qa[0], NULL, &r) != CETTA_OK) die("warm-up eval");
        cetta_embed_result_release(r);
        memset(&v.rt->st, 0, sizeof v.rt->st);
    }
    char route0[256], route1[256];
    route_json(v.space, route0, sizeof route0);
    uint64_t *batch = calloc((size_t)repeat, sizeof(uint64_t));
    size_t answers = 0, items = 0, bad = 0;
    for (int r = 0; r < repeat; r++) {
        uint64_t b0 = cpk_now_ns();
        size_t a = 0, it = 0;
        for (int i = 0; i < nq; i++) {
            cetta_result_t *res = NULL;
            if (cetta_embed_eval(v.e, qa[i], NULL, &res) != CETTA_OK) die("eval");
            if (cetta_embed_result_status(res) != CETTA_OK) die("eval status %s", cetta_embed_status_name(cetta_embed_result_status(res)));
            size_t k = cetta_embed_result_count(res);
            a += k;
            if (r == 0)
                for (size_t j = 0; j < k; j++) {
                    cetta_atom_t *h = NULL;
                    cetta_embed_result_get(res, j, &h);
                    it += cetta_embed_atom_kind(h) == CETTA_ATOM_EXPRESSION ? cetta_embed_atom_arity(h) : 1;
                    bad += bad_answer(p, h);
                    cetta_embed_atom_release(h);
                }
            cetta_embed_result_release(res);
        }
        batch[r] = cpk_now_ns() - b0;
        answers = a;
        if (r == 0) items = it;
    }
    route_json(v.space, route1, sizeof route1);
    uint64_t sorted[1024];
    int rr = repeat < 1024 ? repeat : 1024;
    memcpy(sorted, batch, sizeof(uint64_t) * (size_t)rr);
    qsort(sorted, (size_t)rr, sizeof(uint64_t), cmp_u64);
    struct rusage ru; getrusage(RUSAGE_SELF, &ru);
    char st[1024]; stats_json(v.rt, st, sizeof st);
    printf("{\"mode\":\"%s\",\"site\":\"%s\",\"shape\":\"%s\",\"queries\":%d,\"repeat\":%d,\"answers_per_batch\":%zu,"
           "\"answer_items_first_batch\":%zu,\"bad_answers_first_batch\":%zu,\"batch_ns_median\":%" PRIu64 ",\"batch_ns_min\":%" PRIu64 ",\"batch_ns_max\":%" PRIu64 ","
           "\"per_query_us_median\":%.3f,\"kb_load_ms\":%.1f,%s,\"route_before\":%s,\"route_after\":%s,\"peak_rss_kb\":%ld}\n",
           mode, p->site, fs_shape_name(p->shape), nq, repeat, answers, items, bad, sorted[rr / 2], sorted[0], sorted[rr - 1],
           (double)sorted[rr / 2] / 1000.0 / nq, load_ns / 1e6, st, route0, route1, ru.ru_maxrss);
    for (int i = 0; i < nq; i++) cetta_embed_atom_release(qa[i]);
    free(qa); free(batch); free(qs);
    env_close(&v);
    return 0;
}

/* ---- difftest ---------------------------------------------------------------------- */

static int g_fails;

static Bag *ref_bags(const FsPlan *p, const char *kbtext, char **q, int nq, const char *pre) {
    Env ref = env_open(p, find_mode("fn-generic"), kbtext);
    if (pre) run_text(ref.e, pre);
    Bag *b = calloc((size_t)nq, sizeof(Bag));
    g_eval_error = false;
    char head[256]; mode_head(&ref, head, sizeof head);
    for (int i = 0; i < nq; i++) {
        cetta_atom_t *x = query_atom(&ref, head, q[i]);
        b[i] = eval_bag(ref.e, x);
        cetta_embed_atom_release(x);
    }
    env_close(&ref);
    return b;
}

/* every mode against fn-generic, each in its own engine; path checks */
static void difftest_case(const FsPlan *p, const char *label, const char *kbtext, char **q, int nq, bool mixed) {
    Bag *g = ref_bags(p, kbtext, q, nq, NULL);
    size_t ga = 0;
    for (int i = 0; i < nq; i++) ga += g[i].n;
    if (g_eval_error) { printf("FAIL %s %s reference evaluation error\n", p->site, label); g_fails++; }
    static const char *modes[] = {"fn-hosted", "fn-spec", "fn-fallback"};
    for (size_t mi = 0; mi < 3; mi++) {
        const Mode *m = find_mode(modes[mi]);
        Env v = env_open(p, m, kbtext);
        char head[256]; mode_head(&v, head, sizeof head);
        int same = 0, shown = 0;
        g_eval_error = false;
        for (int i = 0; i < nq; i++) {
            cetta_atom_t *x = query_atom(&v, head, q[i]);
            Bag b = eval_bag(v.e, x);
            if (bag_eq(&g[i], &b)) same++;
            else if (shown++ < 3) {
                fprintf(stderr, "  %s %s %s: mismatch at %s\n", p->site, label, m->name, q[i]);
                show_bag("fn-generic", &g[i]); show_bag(m->name, &b);
            }
            bag_free(&b);
            cetta_embed_atom_release(x);
        }
        const FsStats *s = &v.rt->st;
        bool path = true;
        if (!strcmp(m->name, "fn-spec")) path = mixed ? s->calls == (uint64_t)nq : (s->spec_calls == (uint64_t)nq && s->residual_calls == 0);
        else if (!strcmp(m->name, "fn-fallback")) path = s->forced == (uint64_t)nq && s->residual_calls == (uint64_t)nq && s->spec_calls == 0;
        else path = s->hosted_calls == (uint64_t)nq;
        bool ok = same == nq && path && !g_eval_error;
        char st[1024]; stats_json(v.rt, st, sizeof st);
        printf("%s %-20s %-14s %-11s queries=%d same=%d ref_answers=%zu path=%s%s {%s}\n", ok ? "PASS" : "FAIL", p->site,
               label, m->name, nq, same, ga, path ? "ok" : "WRONG", mixed ? " (mixed: residual allowed)" : "", st);
        if (!path && v.rt->invalid_reason) fprintf(stderr, "  guard: %s\n", v.rt->invalid_reason);
        fflush(stdout);
        if (!ok) g_fails++;
        env_close(&v);
    }
    for (int i = 0; i < nq; i++) bag_free(&g[i]);
    free(g);
}

/* the mutation sequence: the same &self change in a reference engine (fn-generic)
 * and a fn-spec engine, then every query in both; the expected route per step */
typedef enum { EXPECT_SPEC, EXPECT_RESIDUAL, EXPECT_SOME_RESIDUAL } Expect;
typedef struct { const char *label; char *text; Expect expect; } Step;

static char *lhs_text(const FsPlan *p) {
    char *s = malloc(4096);
    int n = snprintf(s, 4096, "(%s", p->function);
    for (unsigned i = 0; i < p->arity; i++) n += snprintf(s + n, 4096 - n, " $a%u", i);
    snprintf(s + n, 4096 - n, ")");
    return s;
}
static char *fmtx(const char *f, ...) {
    va_list ap; va_start(ap, f);
    int n = vsnprintf(NULL, 0, f, ap);
    va_end(ap);
    char *s = malloc((size_t)n + 1);
    va_start(ap, f); vsnprintf(s, (size_t)n + 1, f, ap); va_end(ap);
    return s;
}

static void mutation_test(const FsPlan *p, const char *label, const char *kbtext, char **q, int nq, const char *nfmut) {
    char *lhs = lhs_text(p);
    /* the compiled equation's body, and a redefinition: the same body under a marker */
    const char *eq = p->equation;                     /* (= LHS BODY) */
    const char *body = eq + 3;
    { int d = 0; const char *x = body; do { if (*x == '(') d++; else if (*x == ')') d--; x++; } while (d > 0 && *x); body = x + 1; }
    char *body_c = strndup(body, strlen(body) - 1);
    char *redef = fmtx("(= %.*s (cpk-redefined %s))", (int)(body - eq - 4), eq + 3, body_c);
    const char *shadow_op = p->builtins[p->n_builtins - 1].name;
    unsigned shadow_ar = p->builtins[p->n_builtins - 1].arity;
    char sv[256] = "";
    for (unsigned i = 0, n = 0; i < shadow_ar; i++) n += snprintf(sv + n, sizeof sv - n, " $s%u", i);
    Step steps[12]; int ns = 0;
    steps[ns++] = (Step){"baseline", NULL, EXPECT_SPEC};
    steps[ns++] = (Step){"second-equation-added", fmtx("!(add-atom &self (= %s cpk-extra))", lhs), EXPECT_RESIDUAL};
    steps[ns++] = (Step){"second-equation-removed", fmtx("!(remove-atom &self (= %s cpk-extra))", lhs), EXPECT_SPEC};
    steps[ns++] = (Step){"redefined", fmtx("!(remove-atom &self %s)\n!(add-atom &self %s)", eq, redef), EXPECT_RESIDUAL};
    steps[ns++] = (Step){"restored", fmtx("!(remove-atom &self %s)\n!(add-atom &self %s)", redef, eq), EXPECT_SPEC};
    steps[ns++] = (Step){"builtin-shadowed", fmtx("!(add-atom &self (= (%s%s) cpk-shadow))", shadow_op, sv), EXPECT_RESIDUAL};
    steps[ns++] = (Step){"builtin-unshadowed", fmtx("!(remove-atom &self (= (%s%s) cpk-shadow))", shadow_op, sv), EXPECT_SPEC};
    steps[ns++] = (Step){"declaration-added", fmtx("!(add-atom &self (: %s (-> Atom Atom)))", p->function), EXPECT_RESIDUAL};
    steps[ns++] = (Step){"declaration-removed", fmtx("!(remove-atom &self (: %s (-> Atom Atom)))", p->function), EXPECT_SPEC};
    if (nfmut && *nfmut) {
        steps[ns++] = (Step){"row-not-normal", fmtx("%s", nfmut), EXPECT_SOME_RESIDUAL};
    }
    Env ref = env_open(p, find_mode("fn-generic"), kbtext);
    Env sp = env_open(p, find_mode("fn-spec"), kbtext);
    char rh[256], shd[256];
    mode_head(&ref, rh, sizeof rh); mode_head(&sp, shd, sizeof shd);
    for (int s = 0; s < ns; s++) {
        if (steps[s].text) { run_text(ref.e, steps[s].text); run_text(sp.e, steps[s].text); }
        FsStats before = sp.rt->st;
        int same = 0, shown = 0;
        g_eval_error = false;
        for (int i = 0; i < nq; i++) {
            cetta_atom_t *x = query_atom(&ref, rh, q[i]), *y = query_atom(&sp, shd, q[i]);
            Bag a = eval_bag(ref.e, x), b = eval_bag(sp.e, y);
            if (bag_eq(&a, &b)) same++;
            else if (shown++ < 3) {
                fprintf(stderr, "  %s mutation %s: mismatch at %s\n", p->site, steps[s].label, q[i]);
                show_bag("fn-generic", &a); show_bag("fn-spec", &b);
            }
            bag_free(&a); bag_free(&b);
            cetta_embed_atom_release(x); cetta_embed_atom_release(y);
        }
        uint64_t spec = sp.rt->st.spec_calls - before.spec_calls, res = sp.rt->st.residual_calls - before.residual_calls;
        bool path = steps[s].expect == EXPECT_SPEC ? (spec == (uint64_t)nq && res == 0)
                  : steps[s].expect == EXPECT_RESIDUAL ? (res == (uint64_t)nq && spec == 0)
                  : (res > 0 && spec + res == (uint64_t)nq);
        bool ok = same == nq && path && !g_eval_error;
        printf("%s %-20s %-14s mutation %-24s queries=%d same=%d spec=%" PRIu64 " residual=%" PRIu64 " expect=%s path=%s revalidations=%" PRIu64 "%s%s\n",
               ok ? "PASS" : "FAIL", p->site, label, steps[s].label, nq, same, spec, res,
               steps[s].expect == EXPECT_SPEC ? "spec" : steps[s].expect == EXPECT_RESIDUAL ? "residual" : "some-residual",
               path ? "ok" : "WRONG", sp.rt->st.revalidations, sp.rt->invalid_reason ? " guard: " : "",
               sp.rt->invalid_reason ? sp.rt->invalid_reason : "");
        fflush(stdout);
        if (!ok) g_fails++;
        free(steps[s].text);
    }
    env_close(&ref); env_close(&sp);
    free(lhs); free(body_c); free(redef);
}

/* recursion: a MeTTa function calling the function (fn-generic) or the compiled
 * callable (fn-spec) at the bottom of n nested calls */
static void recursion_test(const FsPlan *p, const char *label, const char *kbtext, char **q, int nq) {
    static const int depths[] = {1, 10, 100, 1000};
    char av[512] = ""; size_t n = 0;
    for (unsigned i = 0; i < p->arity; i++) n += (size_t)snprintf(av + n, sizeof av - n, " $a%u", i);
    Env ref = env_open(p, find_mode("fn-generic"), kbtext);
    Env sp = env_open(p, find_mode("fn-spec"), kbtext);
    char *d1 = fmtx("(= (cpk-depth $n%s) (if (== $n 0) (%s%s) (cpk-depth (- $n 1)%s)))", av, p->function, av, av);
    char *d2 = fmtx("(= (cpk-depth $n%s) (if (== $n 0) (cpk-fnspec:%s%s) (cpk-depth (- $n 1)%s)))", av, p->function, av, av);
    run_text(ref.e, d1); run_text(sp.e, d2);
    int nqq = nq < 6 ? nq : 6;
    for (size_t di = 0; di < sizeof depths / sizeof *depths; di++) {
        FsStats before = sp.rt->st;
        int same = 0, shown = 0;
        g_eval_error = false;
        for (int i = 0; i < nqq; i++) {
            /* (cpk-depth N args...): the query's call with cpk-depth N in front */
            cetta_atom_t *x = query_atom(&ref, "cpk-depth-tmp", q[i]), *y = query_atom(&sp, "cpk-depth-tmp", q[i]);
            cetta_atom_t *ex[2] = {x, y}, *out[2];
            Env *envs[2] = {&ref, &sp};
            for (int k = 0; k < 2; k++) {
                size_t ar = cetta_embed_atom_arity(ex[k]);
                cetta_atom_t *items[20];
                items[0] = sym(envs[k]->e, "cpk-depth");
                cetta_embed_atom_integer(envs[k]->e, depths[di], &items[1]);
                for (size_t j = 1; j < ar; j++) cetta_embed_atom_child(ex[k], j, &items[j + 1]);
                cetta_embed_atom_expression(envs[k]->e, items, ar + 1, &out[k]);
                for (size_t j = 0; j < ar + 1; j++) cetta_embed_atom_release(items[j]);
            }
            Bag a = eval_bag(ref.e, out[0]), b = eval_bag(sp.e, out[1]);
            if (bag_eq(&a, &b)) same++;
            else if (shown++ < 2) {
                fprintf(stderr, "  %s depth %d: mismatch at %s\n", p->site, depths[di], q[i]);
                show_bag("fn-generic", &a); show_bag("fn-spec", &b);
            }
            bag_free(&a); bag_free(&b);
            for (int k = 0; k < 2; k++) { cetta_embed_atom_release(ex[k]); cetta_embed_atom_release(out[k]); }
        }
        uint64_t spec = sp.rt->st.spec_calls - before.spec_calls;
        bool ok = same == nqq && spec == (uint64_t)nqq && !g_eval_error;
        printf("%s %-20s %-14s recursion depth %-5d queries=%d same=%d spec=%" PRIu64 "\n", ok ? "PASS" : "FAIL",
               p->site, label, depths[di], nqq, same, spec);
        fflush(stdout);
        if (!ok) g_fails++;
    }
    free(d1); free(d2);
    env_close(&ref); env_close(&sp);
}

static int cmp_str(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }

static int cmd_difftest(const FsPlan *p, const char *cases, const char *kbfile, const char *qfile) {
    printf("site %s: %s/%u, shape %s, %u pattern(s); origin: %s\n  equation %s\n", p->site, p->function, p->arity,
           fs_shape_name(p->shape), p->n_patterns, p->origin, p->equation);
    char *names[256]; int nn = 0;
    DIR *d = opendir(cases);
    if (!d) die("cases dir %s", cases);
    for (struct dirent *e; (e = readdir(d)) && nn < 256;) {
        size_t l = strlen(e->d_name);
        if (l > 6 && !strcmp(e->d_name + l - 6, ".metta")) names[nn++] = strndup(e->d_name, l - 6);
    }
    closedir(d);
    qsort(names, (size_t)nn, sizeof(char *), cmp_str);
    char path[1024];
    snprintf(path, sizeof path, "%s/nf.mut", cases);
    char *nfmut = cpk_read_file(path, NULL);
    for (int i = 0; i < nn; i++) {
        snprintf(path, sizeof path, "%s/%s.metta", cases, names[i]);
        char *kb = cpk_read_file(path, NULL);
        snprintf(path, sizeof path, "%s/%s.q", cases, names[i]);
        char *qs = cpk_read_file(path, NULL);
        if (!kb || !qs) die("case %s", names[i]);
        bool mixed = strstr(kb, ";; expect: mixed") != NULL;
        static char *lines[4096];
        int nq = read_lines(qs, lines, 4096);
        difftest_case(p, names[i], kb, lines, nq, mixed);
        if (i == 0) {
            mutation_test(p, names[i], kb, lines, nq, nfmut);
            recursion_test(p, names[i], kb, lines, nq);
        }
        free(kb); free(qs); free(names[i]);
    }
    free(nfmut);
    if (kbfile && qfile) {
        char *kb = cpk_read_file(kbfile, NULL), *qs = cpk_read_file(qfile, NULL);
        if (!kb || !qs) die("read inputs");
        static char *lines[8192];
        int nq = read_lines(qs, lines, 8192);
        difftest_case(p, "kb", kb, lines, nq, false);
        free(kb); free(qs);
    }
    printf("summary %s: %d failed\n", p->site, g_fails);
    return g_fails ? 1 : 0;
}

int main(int argc, char **argv) {
    if (argc < 2) die("usage: cettapack-fnspec list|defs|bench|difftest ...");
    const char *site = NULL, *kb = NULL, *q = NULL, *mode = "fn-generic", *cases = NULL;
    int repeat = 5;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--site") && i + 1 < argc) site = argv[++i];
        else if (!strcmp(argv[i], "--kb") && i + 1 < argc) kb = argv[++i];
        else if (!strcmp(argv[i], "--queries") && i + 1 < argc) q = argv[++i];
        else if (!strcmp(argv[i], "--mode") && i + 1 < argc) mode = argv[++i];
        else if (!strcmp(argv[i], "--cases") && i + 1 < argc) cases = argv[++i];
        else if (!strcmp(argv[i], "--repeat") && i + 1 < argc) repeat = atoi(argv[++i]);
        else die("unknown argument %s", argv[i]);
    }
    if (!strcmp(argv[1], "list")) {
        for (const FsPlan *const *p = fs_plans; *p; p++)
            printf("%s %s/%u %s kb=%s queries=%s\n", (*p)->site, (*p)->function, (*p)->arity, fs_shape_name((*p)->shape), (*p)->kb, (*p)->queries);
        return 0;
    }
    const FsPlan *p = site ? fs_find(site) : NULL;
    if (!p) die("unknown or missing --site");
    if (!strcmp(argv[1], "defs")) { printf("%s\n%s\n", p->definitions, p->hosted_def); return 0; }
    if (!strcmp(argv[1], "bench")) { if (!kb || !q) die("bench needs --kb and --queries"); return cmd_bench(p, kb, q, mode, repeat < 1 ? 1 : repeat); }
    if (!strcmp(argv[1], "difftest")) { if (!cases) die("difftest needs --cases"); return cmd_difftest(p, cases, kb, q); }
    die("unknown command %s", argv[1]);
    return 2;
}
