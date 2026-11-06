/* cettapack-host: differential tests and benchmark for CettaPack M1.
 *
 *   cettapack-host test  [--kb FILE]           differential tests (A.9 subset)
 *   cettapack-host bench --kb FILE --queries FILE --mode generic|kernel|fallback
 *                        [--repeat N]           one benchmark process, JSON out
 *   cettapack-host hashes                       live digests, for the manifest check
 */
#define _POSIX_C_SOURCE 200809L
#include "cetta_embed.h"
#include "cetta_embed_native.h"
#include "cpk_pack.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

extern const CpkPlan cpk_plan_cpk_site_target_expressed;
#define PLAN (&cpk_plan_cpk_site_target_expressed)

#ifndef CPK_PACK_DIR
#define CPK_PACK_DIR "packs/target_expressed"
#endif

static const char *g_manifest = CPK_PACK_DIR "/manifest.json";
static const char *g_site = CPK_PACK_DIR "/site.metta";

/* ---- small helpers ---------------------------------------------------------- */

static void die(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    fprintf(stderr, "cettapack-host: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(2);
}

static cetta_engine_t *engine_new(const char *language) {
    cetta_engine_config_t cfg = {.size = sizeof cfg, .abi_version = CETTA_EMBED_ABI_VERSION,
                                 .language = language, .resource_root = CPK_CETTA_ROOT};
    cetta_engine_t *e = NULL;
    cetta_status_t st = cetta_embed_engine_new(&cfg, &e);
    if (st != CETTA_OK) die("engine: %s", cetta_embed_status_name(st));
    return e;
}

static cetta_atom_t *parse1(cetta_engine_t *e, const char *text) {
    cetta_atom_t *a = NULL;
    cetta_status_t st = cetta_embed_parse(e, text, strlen(text), &a);
    if (st != CETTA_OK) die("parse '%s': %s", text, cetta_embed_status_name(st));
    return a;
}

static cetta_atom_t *sym(cetta_engine_t *e, const char *s) {
    cetta_atom_t *a = NULL;
    if (cetta_embed_atom_symbol(e, s, &a) != CETTA_OK) die("symbol");
    return a;
}

static cetta_atom_t *child(cetta_atom_t *a, size_t i) {
    cetta_atom_t *c = NULL;
    if (cetta_embed_atom_child(a, i, &c) != CETTA_OK) die("child");
    return c;
}

static cetta_atom_t *expr(cetta_engine_t *e, cetta_atom_t **items, size_t n) {
    cetta_atom_t *a = NULL;
    if (cetta_embed_atom_expression(e, items, n, &a) != CETTA_OK) die("expression");
    return a;
}

static char *fmt(cetta_atom_t *a) {
    size_t need = 0;
    cetta_embed_atom_format(a, NULL, 0, &need);
    char *s = malloc(need + 1);
    if (!s || cetta_embed_atom_format(a, s, need + 1, &need) != CETTA_OK) die("format");
    return s;
}

/* A result bag: sorted formatted answers (multiset). */
typedef struct { char **items; size_t n; } Bag;

static int cmp_s(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }

static void bag_free(Bag *b) { for (size_t i = 0; i < b->n; i++) free(b->items[i]); free(b->items); b->items = NULL; b->n = 0; }

static cetta_status_t g_last_eval = CETTA_OK;
static char g_last_diag[600];

static Bag eval_bag(cetta_engine_t *e, cetta_atom_t *x) {
    cetta_result_t *r = NULL;
    cetta_status_t st = cetta_embed_eval(e, x, NULL, &r);
    g_last_eval = st;
    g_last_diag[0] = 0;
    if (st == CETTA_OK && cetta_embed_result_status(r) != CETTA_OK) st = g_last_eval = cetta_embed_result_status(r);
    if (st != CETTA_OK) {
        cetta_diagnostic_t d = {.size = sizeof d};
        if (r && cetta_embed_result_diagnostic(r, &d) == CETTA_OK) snprintf(g_last_diag, sizeof g_last_diag, "%s", d.message);
        cetta_embed_result_release(r);
        return (Bag){calloc(1, sizeof(char *)), 0};
    }
    Bag b = {calloc(cetta_embed_result_count(r) + 1, sizeof(char *)), cetta_embed_result_count(r)};
    for (size_t i = 0; i < b.n; i++) {
        cetta_atom_t *h = NULL;
        cetta_embed_result_get(r, i, &h);
        b.items[i] = fmt(h);
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

static Bag bag_support(const Bag *a) {
    Bag b = {calloc(a->n + 1, sizeof(char *)), 0};
    for (size_t i = 0; i < a->n; i++)
        if (i == 0 || strcmp(a->items[i], a->items[i - 1])) b.items[b.n++] = strdup(a->items[i]);
    return b;
}

/* The generic site, written as a user would: (match <space> (, (target RS $gene) (expressed $gene $tissue)) ($gene $tissue)) */
static cetta_atom_t *generic_expr(cetta_engine_t *e, cetta_atom_t *space, const char *rs) {
    char text[512];
    snprintf(text, sizeof text, "(_ (, (target %s $gene) (expressed $gene $tissue)) ($gene $tissue))", rs);
    cetta_atom_t *p = parse1(e, text);
    cetta_atom_t *items[4] = {sym(e, "match"), space, child(p, 1), child(p, 2)};
    cetta_atom_t *x = expr(e, items, 4);
    cetta_embed_atom_release(items[0]); cetta_embed_atom_release(items[2]); cetta_embed_atom_release(items[3]);
    cetta_embed_atom_release(p);
    return x;
}

static cetta_atom_t *site_expr(cetta_engine_t *e, cetta_atom_t *space, const char *rs) {
    cetta_atom_t *items[3] = {sym(e, PLAN->site), space, parse1(e, rs)};
    cetta_atom_t *x = expr(e, items, 3);
    cetta_embed_atom_release(items[0]); cetta_embed_atom_release(items[2]);
    return x;
}

static cetta_atom_t *new_space(cetta_engine_t *e) {
    cetta_space_t *s = NULL;
    if (cetta_embed_space_new(e, &s) != CETTA_OK) die("new space");
    cetta_atom_t *a = NULL;
    cetta_embed_space_atom(s, &a);
    cetta_embed_space_release(s);
    return a;
}

static void add_text(cetta_engine_t *e, cetta_atom_t *space, const char *text) {
    cetta_space_t *s = NULL;
    if (cetta_embed_space_from_atom(e, space, &s) != CETTA_OK) die("space handle");
    const char *p = text;
    while (*p) {
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

static uint64_t space_len(cetta_engine_t *e, cetta_atom_t *space) {
    cetta_space_t *s = NULL;
    uint64_t n = 0;
    cetta_embed_space_from_atom(e, space, &s);
    cetta_embed_space_count(s, &n);
    cetta_embed_space_release(s);
    return n;
}

static CpkPack *open_pack(cetta_engine_t *e, const char *manifest, const char *language,
                          bool pathmap, CpkDemand demand) {
    CpkPackOptions o = {.plan = PLAN, .manifest_path = manifest, .site_path = g_site,
                        .language = language, .pathmap_enabled = pathmap, .demand = demand};
    CpkPack *p = NULL;
    cetta_status_t st = cpk_pack_open(e, &o, &p);
    if (st != CETTA_OK) die("pack open: %s", cetta_embed_status_name(st));
    return p;
}

/* ---- differential tests ----------------------------------------------------- */

typedef struct { int pass, fail; FILE *log; } T;

static void report(T *t, const char *id, const char *name, bool ok, const char *fmtx, ...) {
    char msg[600];
    va_list ap; va_start(ap, fmtx); vsnprintf(msg, sizeof msg, fmtx, ap); va_end(ap);
    printf("%s A.9-%s %-34s %s\n", ok ? "PASS" : "FAIL", id, name, msg);
    if (t->log) fprintf(t->log, "{\"item\":\"%s\",\"name\":\"%s\",\"ok\":%s,\"detail\":\"%s\"}\n", id, name, ok ? "true" : "false", msg);
    if (ok) t->pass++; else t->fail++;
}

static void show_bag(const char *label, const Bag *b) {
    fprintf(stderr, "  %s (%zu):", label, b->n);
    for (size_t i = 0; i < b->n && i < 12; i++) fprintf(stderr, " %s", b->items[i]);
    fprintf(stderr, "%s\n", b->n > 12 ? " ..." : "");
}

typedef struct {
    const char *id, *name, *kb, *rs;
    const char *manifest;        /* NULL: the compiled pack's manifest */
    const char *language;        /* NULL: he */
    bool pathmap;
    CpkDemand demand;
    CpkVerdict want;             /* expected guard verdict; INVALID/PATCHABLE mean fallback */
    long want_n;                 /* expected answer count, -1 to skip */
    const char *extra_after;     /* atoms added after instantiation (dependency change) */
    bool want_loaded;
} Case;

static void run_case(T *t, const Case *c) {
    cetta_engine_t *e = engine_new(c->language ? c->language : "he");
    CpkPack *p = open_pack(e, c->manifest ? c->manifest : g_manifest, c->language ? c->language : "he",
                           c->pathmap, c->demand);
    cetta_atom_t *space = new_space(e);
    add_text(e, space, c->kb);
    if (c->extra_after) {
        /* instantiate on the original space first, then change it */
        cetta_atom_t *x = site_expr(e, space, c->rs);
        Bag warm = eval_bag(e, x);
        bag_free(&warm);
        cetta_embed_atom_release(x);
        add_text(e, space, c->extra_after);
    }
    uint64_t len0 = space_len(e, space);
    cetta_atom_t *gx = generic_expr(e, space, c->rs), *kx = site_expr(e, space, c->rs);
    Bag g = eval_bag(e, gx);
    cetta_status_t gst = g_last_eval;
    char gdiag[600]; snprintf(gdiag, sizeof gdiag, "%s", g_last_diag);
    uint64_t kernel_calls0 = p->stats.kernel_calls, fallbacks0 = p->stats.fallbacks;
    Bag k = eval_bag(e, kx);
    cetta_status_t kst = g_last_eval;
    if (gst != CETTA_OK || kst != CETTA_OK)
        fprintf(stderr, "  eval status: generic=%s (%s) kernel=%s (%s)\n", cetta_embed_status_name(gst), gdiag,
                cetta_embed_status_name(kst), g_last_diag);
    bool used_kernel = p->stats.kernel_calls > kernel_calls0;
    bool fell_back = p->stats.fallbacks > fallbacks0;
    uint64_t len1 = space_len(e, space);
    Bag want = c->demand == CPK_DEMAND_SUPPORT ? bag_support(&g) : (Bag){0};
    const Bag *ref = c->demand == CPK_DEMAND_SUPPORT ? &want : &g;
    bool same = bag_eq(ref, &k);
    bool verdict_ok = p->last_verdict == c->want;
    bool path_ok = c->want == CPK_VALID ? used_kernel && !fell_back : fell_back && !used_kernel;
    bool n_ok = c->want_n < 0 || (long)k.n == c->want_n;
    bool load_ok = p->loaded == c->want_loaded;
    bool ok = same && verdict_ok && path_ok && n_ok && len0 == len1 && load_ok && gst == CETTA_OK && kst == CETTA_OK;
    report(t, c->id, c->name, ok,
           "generic=%zu kernel=%zu same_bag=%s verdict=%s(%s) path=%s loaded=%s space_unchanged=%s",
           ref->n, k.n, same ? "yes" : "NO", cpk_verdict_name(p->last_verdict), p->last_reason,
           used_kernel ? "kernel" : fell_back ? "fallback" : "none", p->loaded ? "yes" : "no",
           len0 == len1 ? "yes" : "NO");
    if (!ok) { show_bag("generic", ref); show_bag("kernel", &k); if (!p->loaded) fprintf(stderr, "  load: %s\n", p->load_error); }
    bag_free(&g); bag_free(&k); bag_free(&want);
    cetta_embed_atom_release(gx); cetta_embed_atom_release(kx); cetta_embed_atom_release(space);
    cpk_pack_close(p);
    cetta_embed_engine_release(e);
}

static char *write_variant_manifest(const char *field, const char *value, const char *name) {
    /* copy the manifest, replacing one string field by a new value (or
     * deleting the line when value is NULL); used to simulate packs built for
     * another profile/structure and packs with a missing hash */
    size_t n = 0;
    char *m = cpk_read_file(g_manifest, &n);
    if (!m) die("read manifest");
    char key[128];
    snprintf(key, sizeof key, "\"%s\": \"", field);
    char *at = strstr(m, key);
    if (!at) die("field %s not in manifest", field);
    char *vstart = at + strlen(key), *vend = strchr(vstart, '"');
    char *path = malloc(512);
    snprintf(path, 512, "%s/%s", getenv("CPK_TMP") ? getenv("CPK_TMP") : "/tmp", name);
    FILE *f = fopen(path, "w");
    if (!f) die("write %s", path);
    if (value) {
        fwrite(m, 1, (size_t)(vstart - m), f);
        fputs(value, f);
        fputs(vend, f);
    } else {
        /* blank the hash: present key, empty value = missing hash */
        fwrite(m, 1, (size_t)(vstart - m), f);
        fputs(vend, f);
    }
    fclose(f);
    free(m);
    return path;
}

static const char *FAKE = "0000000000000000000000000000000000000000000000000000000000000000";

static int cmd_test(const char *kbfile) {
    T t = {0};
    t.log = fopen(getenv("CPK_TEST_LOG") ? getenv("CPK_TEST_LOG") : "/dev/null", "w");
    const char *base =
        "(target rs1 g1)\n(target rs1 g2)\n(target rs2 g3)\n(target rs9 g9)\n"
        "(expressed g1 liver)\n(expressed g1 brain)\n(expressed g2 heart)\n(expressed g3 skin)\n"
        "(interacts g1 g2)\n(annotated g1 go7)\n";
    const char *dups =
        "(target rs1 g1)\n(target rs1 g1)\n(target rs1 g1)\n(target rs1 g2)\n"
        "(expressed g1 liver)\n(expressed g1 liver)\n(expressed g2 heart)\n(expressed g2 lung)\n";
    const char *coref =   /* $gene must be the same gene in both factors */
        "(target rs1 g1)\n(target rs1 g2)\n(expressed g2 heart)\n(expressed g3 liver)\n(expressed g1 g1)\n";
    const char *distinct =  /* $gene and $tissue are distinct variables: equal values allowed */
        "(target rs1 g1)\n(expressed g1 g1)\n(expressed g1 liver)\n";

    /* a 70-byte symbol: MORK would intern it, so the kernel cannot write the
     * key prefix byte for byte and narrows through MORK's query instead */
    const char *longsym =
        "(target rs1 g1)\n(target rs1 gene_with_a_very_long_name_that_mork_interns_rather_than_inlining_xx)\n"
        "(expressed g1 liver)\n(expressed g1 liver)\n(expressed gene_with_a_very_long_name_that_mork_interns_rather_than_inlining_xx heart)\n";
    char *m_profile = write_variant_manifest("digest", FAKE, "cpk-manifest-profile.json");
    char *m_struct = write_variant_manifest("region_hash", FAKE, "cpk-manifest-struct.json");
    char *m_missing = write_variant_manifest("pattern_hash", NULL, "cpk-manifest-missing.json");

    Case cases[] = {
        {"1", "ground match", base, "rs2", NULL, NULL, true, CPK_DEMAND_BAG, CPK_VALID, 1, NULL, true},
        {"2", "no result", base, "rs404", NULL, NULL, true, CPK_DEMAND_BAG, CPK_VALID, 0, NULL, true},
        {"2b", "no result (dangling gene)", base, "rs9", NULL, NULL, true, CPK_DEMAND_BAG, CPK_VALID, 0, NULL, true},
        {"3", "multiple results", base, "rs1", NULL, NULL, true, CPK_DEMAND_BAG, CPK_VALID, 3, NULL, true},
        {"4", "duplicates and multiplicity", dups, "rs1", NULL, NULL, true, CPK_DEMAND_BAG, CPK_VALID, 3 * 2 + 1 * 2, NULL, true},
        {"4s", "support demand dedups", dups, "rs1", NULL, NULL, true, CPK_DEMAND_SUPPORT, CPK_VALID, 3, NULL, true},
        {"5", "co-reference", coref, "rs1", NULL, NULL, true, CPK_DEMAND_BAG, CPK_VALID, 2, NULL, true},
        {"6", "distinct variables", distinct, "rs1", NULL, NULL, true, CPK_DEMAND_BAG, CPK_VALID, 2, NULL, true},
        {"7", "result template", base, "rs1", NULL, NULL, true, CPK_DEMAND_BAG, CPK_VALID, 3, NULL, true},
        {"9", "profile mismatch -> fallback", dups, "rs1", m_profile, NULL, true, CPK_DEMAND_BAG, CPK_INVALID, 8, NULL, true},
        {"10", "schema mismatch (variable record)", "(target rs1 g1)\n(target rs1 $g)\n(expressed g1 liver)\n(expressed g5 brain)\n",
            "rs1", NULL, NULL, true, CPK_DEMAND_BAG, CPK_INVALID, -1, NULL, true},
        {"10b", "schema mismatch (number field)", "(target rs1 g1)\n(target rs1 42)\n(expressed g1 liver)\n(expressed 42 brain)\n",
            "rs1", NULL, NULL, true, CPK_DEMAND_BAG, CPK_INVALID, 2, NULL, true},
        {"10c", "schema mismatch (variable head)", "(target rs1 g1)\n($rel rs1 g7)\n(expressed g1 liver)\n(expressed g7 bone)\n",
            "rs1", NULL, NULL, true, CPK_DEMAND_BAG, CPK_INVALID, -1, NULL, true},
        {"11", "structural-hash mismatch -> fallback", base, "rs1", m_struct, NULL, true, CPK_DEMAND_BAG, CPK_INVALID, 3, NULL, true},
        {"11b", "missing hash -> not loaded (fail closed)", base, "rs1", m_missing, NULL, true, CPK_DEMAND_BAG, CPK_INVALID, 3, NULL, false},
        {"14", "PathMap disabled -> fallback", dups, "rs1", NULL, NULL, false, CPK_DEMAND_BAG, CPK_INVALID, 8, NULL, true},
        {"dep", "space changed after instantiation", base, "rs1", NULL, NULL, true, CPK_DEMAND_BAG, CPK_PATCHABLE, 4, "(expressed g2 kidney)\n", true},
        {"in", "non-symbol input -> fallback", base, "$x", NULL, NULL, true, CPK_DEMAND_BAG, CPK_INVALID, -1, NULL, true},
        {"long", "long symbol: MORK-query narrowing", longsym, "rs1", NULL, NULL, true, CPK_DEMAND_BAG, CPK_VALID, 3, NULL, true},
    };
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) run_case(&t, &cases[i]);

    /* 15: comparison with generic CeTTa over a realistic KB, many seeds */
    if (kbfile) {
        size_t n = 0;
        char *kb = cpk_read_file(kbfile, &n);
        if (!kb) die("read %s", kbfile);
        cetta_engine_t *e = engine_new("he");
        CpkPack *p = open_pack(e, g_manifest, "he", true, CPK_DEMAND_BAG);
        cetta_atom_t *space = new_space(e);
        add_text(e, space, kb);
        free(kb);
        int same = 0, total = 0;
        size_t answers = 0;
        for (int i = 0; i < 300; i++) {
            char rs[32];
            snprintf(rs, sizeof rs, "rs%d", (i * 7919) % 24600);   /* includes absent seeds */
            cetta_atom_t *gx = generic_expr(e, space, rs), *kx = site_expr(e, space, rs);
            Bag g = eval_bag(e, gx), k = eval_bag(e, kx);
            total++; if (bag_eq(&g, &k)) same++; else { fprintf(stderr, "  mismatch at %s\n", rs); show_bag("generic", &g); show_bag("kernel", &k); }
            answers += g.n;
            bag_free(&g); bag_free(&k);
            cetta_embed_atom_release(gx); cetta_embed_atom_release(kx);
        }
        bool ok = same == total && p->stats.fallbacks == 0 && p->stats.kernel_calls == (uint64_t)total;
        report(&t, "15", "generic CeTTa vs kernel, 300 seeds", ok,
               "same=%d/%d answers=%zu kernel_calls=%" PRIu64 " fallbacks=%" PRIu64 " kb_atoms=%" PRIu64,
               same, total, answers, p->stats.kernel_calls, p->stats.fallbacks, space_len(e, space));
        cetta_embed_atom_release(space);
        cpk_pack_close(p);
        cetta_embed_engine_release(e);
    } else {
        report(&t, "15", "generic CeTTa vs kernel, 300 seeds", false, "no --kb given");
    }
    printf("summary: %d passed, %d failed\n", t.pass, t.fail);
    if (t.log) fclose(t.log);
    free(m_profile); free(m_struct); free(m_missing);
    return t.fail ? 1 : 0;
}

/* ---- benchmark -------------------------------------------------------------- */

static int cmp_u64(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return x < y ? -1 : x > y;
}

static int cmd_bench(const char *kbfile, const char *qfile, const char *mode, int repeat) {
    size_t n = 0;
    char *kb = cpk_read_file(kbfile, &n), *qs = cpk_read_file(qfile, NULL);
    if (!kb || !qs) die("read inputs");
    char *queries[4096]; int nq = 0;
    for (char *s = strtok(qs, "\n"); s && nq < 4096; s = strtok(NULL, "\n")) queries[nq++] = s;

    bool kernel = !strcmp(mode, "kernel"), fallback = !strcmp(mode, "fallback"), generic = !strcmp(mode, "generic");
    bool hosted_generic = !strcmp(mode, "hosted-generic");
    if (!kernel && !fallback && !generic && !hosted_generic) die("mode must be generic, hosted-generic, kernel or fallback");
    cetta_engine_t *e = engine_new("he");
    /* fallback mode: a pack whose PathMap narrowing is disabled, so every
     * call takes guard -> INVALID -> residual; it measures the fallback cost.
     * hosted-generic: CeTTa's generic conjunction matcher behind the same
     * callable, so hosting overhead cancels between it and the kernel. */
    CpkPackOptions o = {.plan = PLAN, .manifest_path = g_manifest, .site_path = g_site, .language = "he",
                        .pathmap_enabled = !fallback, .demand = CPK_DEMAND_BAG, .native_generic = hosted_generic};
    CpkPack *p = NULL;
    if (cpk_pack_open(e, &o, &p) != CETTA_OK) die("pack open");
    cetta_atom_t *space = new_space(e);
    uint64_t t0 = cpk_now_ns();
    add_text(e, space, kb);
    uint64_t load_ns = cpk_now_ns() - t0;
    free(kb);

    /* instantiate outside the timed loop (index build is reported apart) */
    if (!generic) {
        cetta_atom_t *x = site_expr(e, space, queries[0]);
        Bag w = eval_bag(e, x); bag_free(&w); cetta_embed_atom_release(x);
        memset(&p->stats, 0, sizeof p->stats);
    }
    uint64_t *batch = calloc((size_t)repeat, sizeof(uint64_t));
    size_t answers = 0;
    for (int r = 0; r < repeat; r++) {
        uint64_t b0 = cpk_now_ns();
        size_t a = 0;
        for (int i = 0; i < nq; i++) {
            cetta_atom_t *x = generic ? generic_expr(e, space, queries[i]) : site_expr(e, space, queries[i]);
            cetta_result_t *res = NULL;
            if (cetta_embed_eval(e, x, NULL, &res) != CETTA_OK) die("eval");
            a += cetta_embed_result_count(res);
            cetta_embed_result_release(res);
            cetta_embed_atom_release(x);
        }
        batch[r] = cpk_now_ns() - b0;
        answers = a;
    }
    uint64_t sorted[1024];
    int rr = repeat < 1024 ? repeat : 1024;
    memcpy(sorted, batch, sizeof(uint64_t) * (size_t)rr);
    qsort(sorted, (size_t)rr, sizeof(uint64_t), cmp_u64);
    struct rusage ru; getrusage(RUSAGE_SELF, &ru);
    double calls = (double)p->stats.calls ? (double)p->stats.calls : 1;
    printf("{\"mode\":\"%s\",\"kb_atoms\":%" PRIu64 ",\"queries\":%d,\"repeat\":%d,"
           "\"answers_per_batch\":%zu,\"batch_ns_median\":%" PRIu64 ",\"batch_ns_min\":%" PRIu64 ",\"batch_ns_max\":%" PRIu64 ","
           "\"per_query_us_median\":%.3f,\"kb_load_ms\":%.1f,\"index_build_ms\":%.1f,"
           "\"index_groups\":%" PRIu64 ",\"index_rows\":%" PRIu64 ",\"calls\":%" PRIu64 ",\"kernel_calls\":%" PRIu64 ",\"fallbacks\":%" PRIu64 ","
           "\"guard_ns_per_call\":%.1f,\"narrow_ns_per_call\":%.1f,\"rematch_ns_per_call\":%.1f,\"answer_ns_per_call\":%.1f,"
           "\"kernel_ns_per_call\":%.1f,\"fallback_ns_per_call\":%.1f,"
           "\"candidates_per_call\":%.3f,\"rematches_per_call\":%.3f,\"rematch_ok_per_call\":%.3f,\"narrow_queries_per_call\":%.3f,"
           "\"last_verdict\":\"%s\",\"last_reason\":\"%s\",\"peak_rss_kb\":%ld}\n",
           mode, space_len(e, space), nq, repeat, answers, sorted[rr / 2], sorted[0], sorted[rr - 1],
           (double)sorted[rr / 2] / 1000.0 / nq, load_ns / 1e6, p->instantiate_ns / 1e6,
           cpk_index_groups(p->index), cpk_index_rows(p->index), p->stats.calls, p->stats.kernel_calls, p->stats.fallbacks,
           p->stats.guard_ns / calls, p->stats.narrow_ns / calls, p->stats.rematch_ns / calls, p->stats.answer_ns / calls,
           p->stats.kernel_ns / calls, p->stats.fallback_ns / calls,
           p->stats.candidates / calls, p->stats.rematches / calls, p->stats.rematch_ok / calls, p->stats.narrow_queries / calls,
           generic ? "-" : cpk_verdict_name(p->last_verdict), generic ? "-" : p->last_reason, ru.ru_maxrss);
    free(batch); free(qs);
    cetta_embed_atom_release(space);
    cpk_pack_close(p);
    cetta_embed_engine_release(e);
    return 0;
}

static int cmd_hashes(void) {
    cetta_engine_t *e = engine_new("he");
    CpkPack *p = open_pack(e, g_manifest, "he", true, CPK_DEMAND_BAG);
    printf("loaded=%s error=%s\n", p->loaded ? "yes" : "no", p->load_error);
    printf("profile  live=%s manifest=%s\n", p->live_profile, p->m_profile ? p->m_profile : "-");
    printf("langdef  live=%s manifest=%s\n", p->live_langdef, p->m_langdef ? p->m_langdef : "-");
    printf("region   live=%s manifest=%s\n", p->live_region, p->m_region ? p->m_region : "-");
    printf("pattern  live=%s manifest=%s\n", p->live_pattern, p->m_pattern ? p->m_pattern : "-");
    printf("template live=%s manifest=%s\n", p->live_template, p->m_template ? p->m_template : "-");
    printf("schema   live=%s manifest=%s\n", p->live_schema, p->m_schema ? p->m_schema : "-");
    printf("digests_match=%s %s\n", p->digests_match ? "yes" : "no", p->digest_mismatch);
    int rc = p->loaded && p->digests_match ? 0 : 1;
    cpk_pack_close(p);
    cetta_embed_engine_release(e);
    return rc;
}

#include "mork_space_bridge_runtime.h"
static int cmd_pmprobe(void) {
    CettaMorkSpaceHandle *pm = cetta_mork_bridge_space_new();
    uint64_t added = 0;
    const char *text = "((target rs5 gene7) 17)\n((target rs5 gene8) 3)\n((expressed gene7 liver) 4)\n";
    if (!cetta_mork_bridge_space_add_text(pm, text, &added)) die("add_text: %s", cetta_mork_bridge_last_error());
    CettaMorkCursorHandle *c = cetta_mork_bridge_cursor_new(pm);
    bool moved = true;
    while (cetta_mork_bridge_cursor_next_val(c, &moved) && moved) {
        uint8_t *b = NULL; size_t n = 0;
        cetta_mork_bridge_cursor_path_bytes(c, &b, &n);
        printf("val:");
        for (size_t i = 0; i < n; i++) printf(" %02x", b[i]);
        printf("  |");
        for (size_t i = 0; i < n; i++) putchar(b[i] >= 32 && b[i] < 127 ? b[i] : '.');
        printf("|\n");
        cetta_mork_bridge_bytes_free(b, n);
    }
    cetta_mork_bridge_cursor_free(c);
    cetta_mork_bridge_space_free(pm);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) die("usage: cettapack-host test|bench|hashes ...");
    if (!strcmp(argv[1], "pmprobe")) return cmd_pmprobe();
    const char *kb = NULL, *q = NULL, *mode = "kernel";
    int repeat = 5;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--kb") && i + 1 < argc) kb = argv[++i];
        else if (!strcmp(argv[i], "--queries") && i + 1 < argc) q = argv[++i];
        else if (!strcmp(argv[i], "--mode") && i + 1 < argc) mode = argv[++i];
        else if (!strcmp(argv[i], "--repeat") && i + 1 < argc) repeat = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--manifest") && i + 1 < argc) g_manifest = argv[++i];
        else if (!strcmp(argv[i], "--site") && i + 1 < argc) g_site = argv[++i];
        else die("unknown argument %s", argv[i]);
    }
    if (!strcmp(argv[1], "test")) return cmd_test(kb);
    if (!strcmp(argv[1], "bench")) { if (!kb || !q) die("bench needs --kb and --queries"); return cmd_bench(kb, q, mode, repeat < 1 ? 1 : repeat); }
    if (!strcmp(argv[1], "hashes")) return cmd_hashes();
    die("unknown command %s", argv[1]);
    return 2;
}
