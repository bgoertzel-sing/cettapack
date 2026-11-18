/* Route probe (t-2026-11-17-cettapack-route-check). Inserted into a COPY of
 * src/cpk_host.c by bench/route-check/build_probe_hosts.sh; the benchmarked
 * hosts are not changed. Active only when CPK_ROUTE_PROBE=1; all output goes
 * to stderr as "route-probe {json}" lines, outside the timed loop.
 *   backend: the space's match backend as CeTTa holds it (Space.match_backend),
 *            native row storage (Space.native.len, dense relation),
 *            PathMap native-shadow sync, CETTA_PATHMAP_QUERY_INDEX
 *   counters: CeTTa runtime counters for the timed batches only, through
 *            (reset-runtime-stats!) before and (get-atoms (runtime-stats!))
 *            after; nonzero ones (stats builds only record them) */
#include "space.h"
#include "space_match_backend.h"

static bool rp_on(void) { const char *v = getenv("CPK_ROUTE_PROBE"); return v && *v && strcmp(v, "0"); }

static void rp_backend(cetta_atom_t *space, const char *phase, const char *mode) {
    if (!rp_on()) return;
    Space *s = cetta_embed_native_space(space);
    if (!s) { fprintf(stderr, "route-probe {\"phase\":\"%s\",\"mode\":\"%s\",\"error\":\"no native space\"}\n", phase, mode); return; }
    const char *env = getenv("CETTA_PATHMAP_QUERY_INDEX");
    int forced_wo_rows = -1;
#ifdef CPK_PROBE_HAS_FORCED_CHECK
    forced_wo_rows = space_match_backend_index_forced_without_rows(s) ? 1 : 0;
#endif
    fprintf(stderr, "route-probe {\"phase\":\"%s\",\"mode\":\"%s\",\"backend\":\"%s\",\"backend_kind\":%d,"
            "\"native_rows\":%llu,\"native_dense\":%s,\"dense_facts\":%llu,\"strict\":%s,\"imported_len\":%llu,"
            "\"pathmap_native_shadow_synced\":%s,\"env_CETTA_PATHMAP_QUERY_INDEX\":\"%s\",\"index_forced_without_rows\":%d}\n",
            phase, mode, space_match_backend_kind_name(s->match_backend.kind), (int)s->match_backend.kind,
            (unsigned long long)s->native.len, s->native.dense ? "true" : "false", (unsigned long long)s->native.dense_facts,
            s->strict ? "true" : "false", (unsigned long long)s->imported_len,
            s->match_backend.kind == SPACE_ENGINE_PATHMAP ? (s->match_backend.pathmap.bridge.native_shadow_synced ? "true" : "false") : "null",
            env ? env : "", forced_wo_rows);
}

/* CeTTa's (reset-runtime-stats!) and (runtime-stats!) are not admitted in
 * the embedded HE profile (they come back unevaluated), so the probe reads the
 * engine's own counters from inside two 0-ary host callbacks: a callback runs
 * inside the engine's scope, where the engine's stats owner is current. */
#include "stats.h"
static const char *rp_mode = "";
static cetta_status_t rp_open(void *ctx, cetta_engine_t *e, cetta_atom_t *const *a, size_t n, void **out) {
    (void)e; (void)a; (void)n; *out = ctx; return CETTA_OK;
}
static cetta_status_t rp_next(void *cur, cetta_callback_state_t *state, cetta_atom_t **out) {
    *out = NULL;
    *state = CETTA_CALLBACK_COMPLETE;
    if (!cur) return CETTA_OK;
    int act = *(int *)cur;
    if (act == 1) {
        cetta_runtime_stats_reset();
        cetta_runtime_stats_enable();
        fprintf(stderr, "route-probe stats-reset %s enabled=%d build_with_stats=%d\n", rp_mode,
                (int)cetta_runtime_stats_is_enabled(), (int)CETTA_BUILD_WITH_RUNTIME_STATS);
    } else {
        CettaRuntimeStats st;
        cetta_runtime_stats_snapshot(&st);
        size_t nz = 0;
        for (uint32_t i = 0; i < CETTA_RUNTIME_COUNTER_COUNT; i++)
            if (st.counters[i]) {
                nz++;
                fprintf(stderr, "route-probe counter %s %s %llu\n", rp_mode,
                        cetta_runtime_counter_name((CettaRuntimeCounter)i), (unsigned long long)st.counters[i]);
            }
        fprintf(stderr, "route-probe counters-summary %s nonzero=%zu of %u build_with_stats=%d\n", rp_mode, nz,
                (unsigned)CETTA_RUNTIME_COUNTER_COUNT, (int)CETTA_BUILD_WITH_RUNTIME_STATS);
    }
    *(int *)cur = 0; /* one action per call */
    return CETTA_OK;
}
static void rp_close(void *cur, cetta_callback_close_mode_t m) { (void)cur; (void)m; }
static void rp_nop(void *ctx) { (void)ctx; }
static int rp_reset_ctx, rp_dump_ctx;

static void rp_call(cetta_engine_t *e, const char *name, int *ctx, int act) {
    static bool registered;
    if (!registered) {
        cetta_atom_t *h1 = NULL, *h2 = NULL;
        cetta_callback_descriptor_t d = {.size = sizeof d, .arity = 0, .kind = CETTA_CALLBACK_STREAM,
            .context = &rp_reset_ctx, .retain = rp_nop, .release = rp_nop, .open = rp_open, .next = rp_next, .close = rp_close};
        if (cetta_embed_register_callback(e, "cpk-rp-reset", &d, &h1) != CETTA_OK) die("register cpk-rp-reset");
        d.context = &rp_dump_ctx;
        if (cetta_embed_register_callback(e, "cpk-rp-dump", &d, &h2) != CETTA_OK) die("register cpk-rp-dump");
        registered = true; /* handles kept for the process */
    }
    *ctx = act;
    cetta_atom_t *x = parse1(e, name);
    cetta_result_t *r = NULL;
    cetta_status_t st = cetta_embed_eval(e, x, NULL, &r);
    if (st != CETTA_OK || *ctx != 0) fprintf(stderr, "route-probe call-failed %s %s status=%s pending=%d\n", rp_mode, name, cetta_embed_status_name(st), *ctx);
    if (r) cetta_embed_result_release(r);
    cetta_embed_atom_release(x);
}

static void rp_begin(cetta_engine_t *e, cetta_atom_t *space, const char *mode) {
    if (!rp_on()) return;
    rp_mode = mode;
    rp_backend(space, "before-timed", mode);
    rp_call(e, "(cpk-rp-reset)", &rp_reset_ctx, 1);
}

static void rp_end(cetta_engine_t *e, cetta_atom_t *space, const char *mode) {
    if (!rp_on()) return;
    rp_call(e, "(cpk-rp-dump)", &rp_dump_ctx, 2);
    rp_backend(space, "after-timed", mode);
}
