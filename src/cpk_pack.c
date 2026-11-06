/* CettaPack M1: pack loading, guard, hosted callable, residual fallback. */
#define _POSIX_C_SOURCE 200809L
#include "cpk_pack.h"

#include "cetta_embed_native.h"
#include "match.h"
#include "space_match_backend.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CPK_CETTA_COMMIT
#error "CPK_CETTA_COMMIT must name the CeTTa pin"
#endif
#ifndef CPK_PATCHES_SHA256
#error "CPK_PATCHES_SHA256 must name the applied patch series"
#endif
#ifndef CPK_BUILD_MODE
#error "CPK_BUILD_MODE must name the CeTTa BUILD mode"
#endif
#ifndef CPK_CETTA_ROOT
#error "CPK_CETTA_ROOT must name the CeTTa source tree used for the build"
#endif

const char *cpk_verdict_name(CpkVerdict v) {
    switch (v) {
    case CPK_VALID: return "VALID";
    case CPK_INVALID: return "INVALID";
    case CPK_PATCHABLE: return "PATCHABLE";
    case CPK_REPLAN: return "REPLAN";
    }
    return "?";
}

/* ---- digests --------------------------------------------------------------- */

typedef struct { char *buf; size_t len, cap; } SBuf;
static bool sb_put(SBuf *b, const char *s) {
    size_t n = strlen(s);
    if (b->len + n + 1 > b->cap) {
        size_t c = b->cap ? b->cap : 256;
        while (b->len + n + 1 > c) c *= 2;
        char *p = realloc(b->buf, c);
        if (!p) return false;
        b->buf = p; b->cap = c;
    }
    memcpy(b->buf + b->len, s, n + 1);
    b->len += n;
    return true;
}

void cpk_live_profile_digest(const char *language, char out[65], char *components, size_t len) {
    char text[512];
    snprintf(text, sizeof text,
             "cpk-profile-v1\ncetta=%s\npatches=%s\nbuild=%s\nlanguage=%s\n",
             CPK_CETTA_COMMIT, CPK_PATCHES_SHA256, CPK_BUILD_MODE, language ? language : "");
    cpk_sha256_hex(text, strlen(text), out);
    if (components) snprintf(components, len, "%s", text);
}

void cpk_plan_schema_digest(const CpkPlan *plan, char out[65]) {
    cpk_plan_schema_digest_with(plan, NULL, out);
}

void cpk_plan_schema_digest_with(const CpkPlan *plan, const char *extra, char out[65]) {
    /* relations in conjunction order: head/arity:sym,... */
    SBuf b = {0};
    sb_put(&b, "cpk-schema-v1\n");
    for (uint8_t ci = 0; ci < plan->nfactors; ci++) {
        for (uint8_t k = 0; k < plan->nfactors; k++) {
            const CpkFactor *f = &plan->factors[k];
            if (f->conj_index != ci) continue;
            char line[128];
            snprintf(line, sizeof line, "%s/%u:", f->fields[0].text, (unsigned)f->arity);
            sb_put(&b, line);
            for (uint8_t i = 1; i < f->arity; i++) sb_put(&b, i > 1 ? ",sym" : "sym");
            sb_put(&b, "\n");
        }
    }
    if (extra && *extra) sb_put(&b, extra);
    cpk_sha256_hex(b.buf, b.len, out);
    free(b.buf);
}

/* cpk-struct-v1 canonical text, built through the public embedding API:
 *   symbol -> its text; variable -> $_<n>, numbered by first occurrence in
 *   the hashed unit; expression -> "(" children joined by one space ")";
 *   any other atom -> CeTTa's formatted text.
 * Digest: sha256("cpk-struct-v1\n" + canonical). */
typedef struct { char *names[64]; unsigned n; } VarNames;

static bool canon_rec(const cetta_atom_t *a, SBuf *b, VarNames *vn) {
    cetta_atom_kind_t k = cetta_embed_atom_kind(a);
    char text[512] = {0};
    size_t need = 0;
    if (k == CETTA_ATOM_EXPRESSION) {
        size_t n = cetta_embed_atom_arity(a);
        if (!sb_put(b, "(")) return false;
        for (size_t i = 0; i < n; i++) {
            cetta_atom_t *c = NULL;
            if (cetta_embed_atom_child(a, i, &c) != CETTA_OK) return false;
            bool ok = (i == 0 || sb_put(b, " ")) && canon_rec(c, b, vn);
            cetta_embed_atom_release(c);
            if (!ok) return false;
        }
        return sb_put(b, ")");
    }
    if (k == CETTA_ATOM_VARIABLE || k == CETTA_ATOM_SYMBOL) {
        if (cetta_embed_atom_get_text(a, text, sizeof text, &need) != CETTA_OK) return false;
        if (k == CETTA_ATOM_SYMBOL) return sb_put(b, text);
        unsigned idx = vn->n;
        for (unsigned i = 0; i < vn->n; i++) if (!strcmp(vn->names[i], text)) { idx = i; break; }
        if (idx == vn->n) { if (vn->n == 64) return false; vn->names[vn->n++] = strdup(text); }
        char v[32];
        snprintf(v, sizeof v, "$_%u", idx);
        return sb_put(b, v);
    }
    if (cetta_embed_atom_format(a, text, sizeof text, &need) != CETTA_OK) return false;
    return sb_put(b, text);
}

static bool canon_handle(const cetta_atom_t *a, char out[65], char **canon_out) {
    SBuf b = {0};
    VarNames vn = {0};
    bool ok = sb_put(&b, "cpk-struct-v1\n") && canon_rec(a, &b, &vn);
    for (unsigned i = 0; i < vn.n; i++) free(vn.names[i]);
    if (ok) {
        cpk_sha256_hex(b.buf, b.len, out);
        if (canon_out) *canon_out = strdup(b.buf + strlen("cpk-struct-v1\n"));
    }
    free(b.buf);
    return ok;
}

/* sha256(tag "\n" text): one domain tag per identity field (A.5) */
static void tagged_hash(const char *tag, const char *text, char out[65]) {
    size_t n = strlen(tag) + 1 + strlen(text);
    char *buf = malloc(n + 1);
    if (!buf) { out[0] = 0; return; }
    snprintf(buf, n + 1, "%s\n%s", tag, text);
    cpk_sha256_hex(buf, n, out);
    free(buf);
}

static cetta_atom_t *child(const cetta_atom_t *a, size_t i) {
    cetta_atom_t *c = NULL;
    if (!a || cetta_embed_atom_kind(a) != CETTA_ATOM_EXPRESSION ||
        cetta_embed_atom_arity(a) <= i || cetta_embed_atom_child(a, i, &c) != CETTA_OK)
        return NULL;
    return c;
}

/* A symbol, or a token CeTTa reads as a builtin (=, match), by its text. */
static bool sym_is(const cetta_atom_t *a, const char *s) {
    char text[256] = {0}; size_t need = 0;
    if (!a) return false;
    cetta_atom_kind_t k = cetta_embed_atom_kind(a);
    if (k == CETTA_ATOM_SYMBOL)
        return cetta_embed_atom_get_text(a, text, sizeof text, &need) == CETTA_OK && !strcmp(text, s);
    if (k == CETTA_ATOM_EXPRESSION || k == CETTA_ATOM_VARIABLE) return false;
    return cetta_embed_atom_format(a, text, sizeof text, &need) == CETTA_OK && !strcmp(text, s);
}

/* ---- loading ---------------------------------------------------------------- */

static void collect_vars(const cetta_atom_t *a, VarNames *vn, const cetta_atom_t **handles,
                         cetta_atom_t **owned, unsigned *nowned) {
    cetta_atom_kind_t k = cetta_embed_atom_kind(a);
    if (k == CETTA_ATOM_VARIABLE) {
        char text[256] = {0}; size_t need = 0;
        if (cetta_embed_atom_get_text(a, text, sizeof text, &need) != CETTA_OK) return;
        for (unsigned i = 0; i < vn->n; i++) if (!strcmp(vn->names[i], text)) return;
        if (vn->n < CPK_MAX_SLOTS) { handles[vn->n] = a; vn->names[vn->n++] = strdup(text); }
        return;
    }
    if (k != CETTA_ATOM_EXPRESSION) return;
    size_t n = cetta_embed_atom_arity(a);
    for (size_t i = 0; i < n; i++) {
        cetta_atom_t *c = NULL;
        if (cetta_embed_atom_child(a, i, &c) != CETTA_OK) continue;
        owned[(*nowned)++] = c;           /* keep alive while handles point into it */
        collect_vars(c, vn, handles, owned, nowned);
    }
}

static bool pack_load(CpkPack *p, char *err, size_t errlen) {
    const CpkPlan *plan = p->opt.plan;

    /* the region, parsed by CeTTa */
    size_t slen = 0;
    char *src = cpk_read_file(p->opt.site_path, &slen);
    if (!src) { snprintf(err, errlen, "cannot read site source %s", p->opt.site_path); return false; }
    cetta_status_t st = cetta_embed_parse(p->engine, src, slen, &p->region_handle);
    free(src);
    if (st != CETTA_OK) { snprintf(err, errlen, "site source does not parse: %s", cetta_embed_status_name(st)); return false; }

    /* region shape: (= (<site> $space ...) (match $space (, f1 ... fn) template)) */
    cetta_atom_t *eq = child(p->region_handle, 0), *lhs = child(p->region_handle, 1),
                 *body = child(p->region_handle, 2);
    cetta_atom_t *head = child(lhs, 0), *mhead = child(body, 0), *conj = child(body, 2),
                 *tmpl = child(body, 3), *comma = child(conj, 0);
    bool shape = sym_is(eq, "=") && sym_is(head, plan->site) && sym_is(mhead, "match") &&
                 sym_is(comma, ",") && tmpl &&
                 cetta_embed_atom_arity(conj) == (size_t)plan->nfactors + 1 &&
                 cetta_embed_atom_arity(lhs) == (size_t)plan->ninputs + 1;
    bool ok = shape;
    if (!shape) {
        char text[600]; size_t need = 0;
        if (cetta_embed_atom_format(p->region_handle, text, sizeof text, &need) != CETTA_OK) snprintf(text, sizeof text, "?");
        char et[64] = "", ft[64] = ""; size_t n2 = 0;
        cetta_status_t s1 = eq ? cetta_embed_atom_get_text(eq, et, sizeof et, &n2) : CETTA_INVALID_ARGUMENT;
        cetta_status_t s2 = eq ? cetta_embed_atom_format(eq, ft, sizeof ft, &n2) : CETTA_INVALID_ARGUMENT;
        snprintf(err, errlen, "site source does not have the compiled region's shape: %s [eq=%d head=%d match=%d comma=%d] eqkind=%d text(%s)=%s fmt(%s)=%s",
                 text, sym_is(eq, "="), sym_is(head, plan->site), sym_is(mhead, "match"), sym_is(comma, ","),
                 eq ? (int)cetta_embed_atom_kind(eq) : -1, cetta_embed_status_name(s1), et, cetta_embed_status_name(s2), ft);
    }
    if (ok) {
        char dummy[65], *rc = NULL, *pc = NULL, *tc = NULL;
        ok = canon_handle(p->region_handle, dummy, &rc) && canon_handle(conj, dummy, &pc) &&
             canon_handle(tmpl, dummy, &tc);
        if (ok) {
            tagged_hash("cpk-normalized-v1", rc, p->live_normalized);
            tagged_hash("cpk-meta-expanded-v1", rc, p->live_region);   /* meta-expansion = identity in M1 */
            tagged_hash("cpk-pattern-v1", pc, p->live_pattern);
            tagged_hash("cpk-template-v1", tc, p->live_template);
        }
        free(rc); free(pc); free(tc);
    }

    /* slots, factor and template atoms, borrowed from the parsed region */
    if (ok) {
        VarNames vn = {0};
        const cetta_atom_t *vh[CPK_MAX_SLOTS] = {0};
        cetta_atom_t *owned[512]; unsigned nowned = 0;
        collect_vars(p->region_handle, &vn, vh, owned, &nowned);
        if (vn.n != plan->nslots) { ok = false; snprintf(err, errlen, "region has %u variables, plan %u", vn.n, (unsigned)plan->nslots); }
        for (unsigned i = 0; ok && i < vn.n; i++) {
            const char *want = plan->slot_names[i] + (plan->slot_names[i][0] == '$');
            if (strcmp(vn.names[i], want)) { ok = false; snprintf(err, errlen, "slot %u is %s, plan says %s", i, vn.names[i], plan->slot_names[i]); }
            p->slot_vars[i] = (Atom *)cetta_embed_native_atom(vh[i]);
        }
        for (unsigned i = 0; i < vn.n; i++) free(vn.names[i]);
        for (unsigned i = 0; i < nowned; i++) cetta_embed_atom_release(owned[i]);
    }
    if (ok) {
        p->conj_atom = (Atom *)cetta_embed_native_atom(conj);
        p->template_atom = (Atom *)cetta_embed_native_atom(tmpl);
        for (uint8_t k = 0; ok && k < plan->nfactors; k++) {
            const CpkFactor *f = &plan->factors[k];
            Atom *fa = p->conj_atom->expr.elems[1 + f->conj_index];
            p->factor_atoms[k] = fa;
            /* the plan must describe this factor exactly */
            ok = fa->kind == ATOM_EXPR && fa->expr.len == f->arity;
            cetta_atom_t *fh = child(conj, 1 + f->conj_index);
            for (uint8_t i = 0; ok && i < f->arity; i++) {
                cetta_atom_t *c = child(fh, i);
                if (f->fields[i].kind == CPK_FIELD_CONST) ok = sym_is(c, f->fields[i].text);
                else ok = cetta_embed_atom_kind(c) == CETTA_ATOM_VARIABLE &&
                          fa->expr.elems[i]->kind == ATOM_VAR &&
                          var_id_eq(fa->expr.elems[i]->var_id, p->slot_vars[f->fields[i].slot]->var_id);
                cetta_embed_atom_release(c);
            }
            cetta_embed_atom_release(fh);
            if (!ok) snprintf(err, errlen, "plan factor %u does not match the region", (unsigned)k);
        }
    }
    cetta_embed_atom_release(eq); cetta_embed_atom_release(lhs); cetta_embed_atom_release(body);
    cetta_embed_atom_release(head); cetta_embed_atom_release(mhead); cetta_embed_atom_release(conj);
    cetta_embed_atom_release(tmpl); cetta_embed_atom_release(comma);
    if (!ok) return false;

    p->region_ok = true;

    /* the manifest: fail closed */
    size_t mlen = 0;
    char *mtext = cpk_read_file(p->opt.manifest_path, &mlen);
    if (!mtext) { snprintf(err, errlen, "cannot read manifest %s", p->opt.manifest_path); return false; }
    p->manifest = cpk_json_parse(mtext);
    free(mtext);
    if (!p->manifest) { snprintf(err, errlen, "manifest is not valid JSON"); return false; }
    const char *kind = cpk_json_str(p->manifest, "artifact_kind");
    const char *version = cpk_json_str(p->manifest, "manifest_version");
    if (!kind || strcmp(kind, "cettapack") || !version || strcmp(version, "0.2")) {
        snprintf(err, errlen, "not a CettaPack manifest v0.2 (A.2)"); return false;
    }
    const char *site = cpk_json_str(p->manifest, "x_cettapack.site.name");
    if (!site || strcmp(site, plan->site)) { snprintf(err, errlen, "manifest names another site"); return false; }

    /* fail closed: every hash must be present and well formed */
    struct { const char *path; const char **dst; } req[] = {
        {"identity.profile_digest", &p->m_profile}, {"identity.langdef_pack_digest", &p->m_langdef},
        {"identity.source_span_hash", &p->m_span}, {"identity.normalized_region_hash", &p->m_normalized},
        {"identity.meta_expanded_region_hash", &p->m_region}, {"identity.match_pattern_hash", &p->m_pattern},
        {"x_cettapack.template_hash", &p->m_template}, {"spaces[0].schema_hash", &p->m_schema},
    };
    const CpkJson *spaces = cpk_json_get(p->manifest, "spaces");
    const CpkJson *space0 = spaces && spaces->kind == CPK_JSON_ARRAY && spaces->n > 0 ? spaces->items[0] : NULL;
    for (size_t i = 0; i < sizeof req / sizeof *req; i++) {
        const char *v = strncmp(req[i].path, "spaces[0].", 10)
            ? cpk_json_str(p->manifest, req[i].path)
            : (space0 ? cpk_json_str(space0, req[i].path + 10) : NULL);
        bool hex = v && strlen(v) == 64;
        for (size_t c = 0; hex && c < 64; c++)
            hex = (v[c] >= '0' && v[c] <= '9') || (v[c] >= 'a' && v[c] <= 'f');
        if (!hex) { snprintf(err, errlen, "missing or malformed hash %s: not loading", req[i].path); return false; }
        *req[i].dst = v;
    }
    const char *mult = space0 ? cpk_json_str(space0, "multiplicity_source") : NULL;
    const char *policy = space0 ? cpk_json_str(space0, "revision_policy") : NULL;
    if (!space0 || !cpk_json_str(space0, "space_id") || !mult || strcmp(mult, "space_records") || !policy) {
        snprintf(err, errlen, "missing or unsupported space dependency entry: not loading");
        return false;
    }
    {
        /* source span hash: the site source bytes as read now */
        size_t n = 0;
        char *bytes = cpk_read_file(p->opt.site_path, &n);
        if (!bytes) { snprintf(err, errlen, "cannot read site source"); return false; }
        char *buf = malloc(n + 12);
        if (!buf) { free(bytes); return false; }
        memcpy(buf, "cpk-span-v1\n", 12);
        memcpy(buf + 12, bytes, n);
        cpk_sha256_hex(buf, n + 12, p->live_span);
        free(buf); free(bytes);
    }

    /* live digests */
    cpk_live_profile_digest(p->opt.language, p->live_profile, NULL, 0);
    char langdef_root[1024];
    snprintf(langdef_root, sizeof langdef_root, "%s/langdef/%s", CPK_CETTA_ROOT,
             cpk_json_str(p->manifest, "x_cettapack.langdef_tree") ? cpk_json_str(p->manifest, "x_cettapack.langdef_tree") : "he");
    if (!cpk_tree_digest(langdef_root, p->live_langdef)) { snprintf(err, errlen, "cannot digest %s", langdef_root); return false; }
    cpk_plan_schema_digest(plan, p->live_schema);

    /* mismatches do not refuse loading: they make the guard fail */
    struct { const char *name, *live, *want; } cmp[] = {
        {"profile", p->live_profile, p->m_profile}, {"langdef", p->live_langdef, p->m_langdef},
        {"normalized region hash", p->live_normalized, p->m_normalized},
        {"meta-expanded region hash", p->live_region, p->m_region},
        {"pattern structural hash", p->live_pattern, p->m_pattern},
        {"template structural hash", p->live_template, p->m_template},
        {"schema", p->live_schema, p->m_schema},
    };
    p->digests_match = true;
    for (size_t i = 0; i < sizeof cmp / sizeof *cmp; i++)
        if (strcmp(cmp[i].live, cmp[i].want)) {
            if (p->digests_match) snprintf(p->digest_mismatch, sizeof p->digest_mismatch, "%s mismatch", cmp[i].name);
            p->digests_match = false;
        }
    return true;
}

/* ---- the guard -------------------------------------------------------------- */

static CpkVerdict guard(CpkPack *p, Space *s, Atom *const *inputs, char *why, size_t len) {
    const CpkPlan *plan = p->opt.plan;
    if (!p->loaded) { snprintf(why, len, "pack not loaded (fail closed): %s", p->load_error); return CPK_INVALID; }
    /* profile, langdef, structural hashes and schema digest, compared every call */
    if (strcmp(p->live_profile, p->m_profile)) { snprintf(why, len, "profile mismatch"); return CPK_INVALID; }
    if (strcmp(p->live_langdef, p->m_langdef)) { snprintf(why, len, "langdef mismatch"); return CPK_INVALID; }
    if (strcmp(p->live_region, p->m_region)) { snprintf(why, len, "meta-expanded region hash mismatch"); return CPK_INVALID; }
    if (strcmp(p->live_pattern, p->m_pattern)) { snprintf(why, len, "match pattern hash mismatch"); return CPK_INVALID; }
    /* not in A.3, checked as well: the residual and the answers depend on them */
    if (strcmp(p->live_template, p->m_template) || strcmp(p->live_normalized, p->m_normalized)) {
        snprintf(why, len, "template or normalized region hash mismatch"); return CPK_INVALID;
    }
    /* the compiled plan's schema must be the one the manifest was built for */
    if (strcmp(p->live_schema, p->m_schema)) { snprintf(why, len, "schema digest mismatch (compiled plan)"); return CPK_INVALID; }
    if (!s) { snprintf(why, len, "space input is not a space"); return CPK_INVALID; }
    for (uint8_t i = 0; i < plan->ninputs; i++) {
        if (plan->input_slot[i] < 0) continue;
        if (!inputs[i] || inputs[i]->kind != ATOM_SYMBOL) { snprintf(why, len, "input %u is not a ground symbol", (unsigned)i); return CPK_INVALID; }
    }
    if (!space_match_backend_supports_seeded_candidates(s)) { snprintf(why, len, "space backend does not take seeded candidates"); return CPK_INVALID; }
    if (!p->index) { snprintf(why, len, "no narrowing index"); return CPK_INVALID; }
    /* space dependency vector: the instantiated space, at the indexed revision */
    if (cpk_index_space(p->index) != s || cpk_index_instance_id(p->index) != space_instance_id(s)) { snprintf(why, len, "space differs from the instantiated dependency"); return CPK_INVALID; }
    /* revision policy append_only_epoch (A.3 check_revision_policy) */
    uint64_t delta = 0;
    switch (cpk_index_change(p->index, s, &delta)) {
    case CPK_CHANGE_NONE: break;
    case CPK_CHANGE_DESTRUCTIVE:
        snprintf(why, len, "destructive write since instantiation"); return CPK_INVALID;
    case CPK_CHANGE_APPEND:
        if (delta > p->max_append_delta) {
            snprintf(why, len, "append of %llu atoms over the patch budget %llu", (unsigned long long)delta, (unsigned long long)p->max_append_delta);
            return CPK_REPLAN;
        }
        snprintf(why, len, "append-only delta of %llu atoms", (unsigned long long)delta);
        return CPK_PATCHABLE;
    }
    /* per-space schema hash (A.3): the live space's observed schema digest */
    if (strcmp(cpk_index_space_schema(p->index), p->m_schema)) {
        snprintf(why, len, "schema: live space schema hash mismatch (%s)", cpk_index_schema_note(p->index)); return CPK_INVALID;
    }
    if (!p->opt.pathmap_enabled || !cpk_index_pathmap_ok(p->index)) { snprintf(why, len, "PathMap narrowing disabled or unavailable"); return CPK_INVALID; }
    snprintf(why, len, "ok");
    return CPK_VALID;
}

/* ---- callable -------------------------------------------------------------- */

typedef struct {
    CpkPack *pack;
    cetta_engine_t *engine;
    cetta_atom_t *const *args;
    size_t nargs;
    cetta_atom_t **answers;
    size_t n, cap, pos;
    Atom **raw;                    /* kernel answers, borrowed from the call's scratch arena */
    size_t nraw, rawcap;
    bool computed, failed;
} Cursor;

static int push_handle(Cursor *c, cetta_atom_t *h) {
    if (c->n == c->cap) {
        size_t cap = c->cap ? c->cap * 2 : 16;
        cetta_atom_t **t = realloc(c->answers, cap * sizeof(*t));
        if (!t) return -1;
        c->answers = t; c->cap = cap;
    }
    c->answers[c->n++] = h;
    return 0;
}

static int sink_emit(void *sink, Atom *answer) {
    Cursor *c = sink;
    if (c->nraw == c->rawcap) {
        size_t cap = c->rawcap ? c->rawcap * 2 : 16;
        Atom **t = realloc(c->raw, cap * sizeof(*t));
        if (!t) return -1;
        c->raw = t; c->rawcap = cap;
    }
    c->raw[c->nraw++] = answer;
    return 0;
}

/* The pack's A.6 visitor: a CeTTa result bag carries multiplicity as
 * repeated occurrences, so an answer of multiplicity m becomes m results. */
static CpkVisitCtl visit_answer(void *user_data, Atom *answer, uint64_t multiplicity) {
    for (uint64_t i = 0; i < multiplicity; i++)
        if (sink_emit(user_data, answer) != 0) return CPK_VISIT_ERROR;
    return CPK_VISIT_CONTINUE;
}

/* Answer construction: one deep copy of all answers into an engine-owned
 * handle, then one handle per answer occurrence (children of that copy). */
static int materialize(Cursor *c, Arena *scratch) {
    if (c->nraw == 0) return 0;
    cetta_atom_t *bundle = NULL;
    if (cetta_embed_native_atom_copy(c->engine, atom_expr(scratch, c->raw, c->nraw), &bundle) != CETTA_OK) return -1;
    int rc = 0;
    for (size_t i = 0; !rc && i < c->nraw; i++) {
        cetta_atom_t *h = NULL;
        rc = cetta_embed_atom_child(bundle, i, &h) == CETTA_OK ? push_handle(c, h) : -1;
    }
    cetta_embed_atom_release(bundle);
    return rc;
}

static void drop_answers(Cursor *c) {
    for (size_t i = 0; i < c->n; i++) cetta_embed_atom_release(c->answers[i]);
    c->n = 0;
}

/* The residual: generic CeTTa evaluating the source region's match with the
 * inputs substituted, (match <space> <conjunction> <template>). */
static bool residual(Cursor *c, Atom *const *inputs) {
    CpkPack *p = c->pack;
    const CpkPlan *plan = p->opt.plan;
    uint64_t t0 = cpk_now_ns();
    bool ok = p->region_ok;
    cetta_result_t *result = NULL;
    cetta_atom_t *expr = NULL;
    if (ok && p->region_handle && p->conj_atom) {
        Arena a;
        arena_init_detached(&a);
        Bindings env;
        bindings_init(&env);
        for (uint8_t i = 0; ok && i < plan->ninputs; i++)
            if (plan->input_slot[i] >= 0)
                ok = bindings_add_var(&env, p->slot_vars[plan->input_slot[i]], inputs[i]);
        if (ok) {
            Atom *elems[4];
            elems[0] = atom_symbol(&a, "match");
            elems[1] = inputs[plan->space_input];
            elems[2] = bindings_apply(&env, &a, p->conj_atom);
            elems[3] = bindings_apply(&env, &a, p->template_atom);
            ok = elems[2] && elems[3] &&
                 cetta_embed_native_atom_copy(c->engine, atom_expr(&a, elems, 4), &expr) == CETTA_OK;
        }
        bindings_free(&env);
        arena_free(&a);
    } else {
        ok = false;
    }
    if (ok) ok = cetta_embed_eval(c->engine, expr, NULL, &result) == CETTA_OK &&
                 cetta_embed_result_status(result) == CETTA_OK;
    for (size_t i = 0; ok && i < cetta_embed_result_count(result); i++) {
        cetta_atom_t *h = NULL;
        ok = cetta_embed_result_get(result, i, &h) == CETTA_OK && push_handle(c, h) == 0;
    }
    cetta_embed_result_release(result);
    cetta_embed_atom_release(expr);
    p->stats.fallbacks++;
    p->stats.fallback_ns += cpk_now_ns() - t0;
    return ok;
}

static void compute(Cursor *c) {
    CpkPack *p = c->pack;
    const CpkPlan *plan = p->opt.plan;
    p->stats.calls++;
    Atom *inputs[CPK_MAX_SLOTS] = {0};
    for (size_t i = 0; i < c->nargs && i < CPK_MAX_SLOTS; i++)
        inputs[i] = (Atom *)cetta_embed_native_atom(c->args[i]);
    Space *s = cetta_embed_native_space(c->args[plan->space_input]);

    /* instantiation: the narrowing index for this space, built once */
    if (p->loaded && p->digests_match && s && !p->index && !p->opt.native_generic) {
        uint64_t t0 = cpk_now_ns();
        Arena a;
        arena_init_detached(&a);
        char why[200] = "";
        p->index = cpk_index_build(plan, s, &a, p->opt.pathmap_enabled, why, sizeof why);
        arena_free(&a);
        p->instantiate_ns += cpk_now_ns() - t0;
    }

    if (p->opt.native_generic && s && p->region_ok) {
        uint64_t k0 = cpk_now_ns();
        Arena a;
        arena_init_detached(&a);
        Bindings seed;
        bindings_init(&seed);
        bool ok = true;
        for (uint8_t i = 0; ok && i < plan->ninputs; i++)
            if (plan->input_slot[i] >= 0)
                ok = bindings_add_var(&seed, p->slot_vars[plan->input_slot[i]], inputs[i]);
        BindingSet out;
        if (ok) {
            Atom *pats[CPK_MAX_FACTORS];
            for (uint8_t k = 0; k < plan->nfactors; k++) pats[k] = p->conj_atom->expr.elems[1 + k];
            space_query_conjunction(s, &a, pats, plan->nfactors, &seed, &out);
            for (CettaCount i = 0; ok && i < out.len; i++) {
                Atom *ans = bindings_apply(&out.items[i], &a, p->template_atom);
                ok = ans && sink_emit(c, ans) == 0;
            }
            binding_set_free(&out);
        }
        if (ok) ok = materialize(c, &a) == 0;
        c->nraw = 0;
        bindings_free(&seed);
        arena_free(&a);
        p->stats.kernel_ns += cpk_now_ns() - k0;
        if (!ok) { drop_answers(c); c->failed = true; }
        return;
    }

    uint64_t g0 = cpk_now_ns();
    CpkVerdict v = guard(p, s, inputs, p->last_reason, sizeof p->last_reason);
    p->stats.guard_ns += cpk_now_ns() - g0;
    p->last_verdict = v;

    if (v == CPK_PATCHABLE) {
        /* A.4: apply_patches is an M1 stub that applies nothing, so the
         * re-check cannot reach VALID: schedule replan (stub) and fall back */
        p->stats.patch_attempts++;
        snprintf(p->last_reason + strlen(p->last_reason), sizeof p->last_reason - strlen(p->last_reason),
                 "; patching is an M1 stub: fallback");
    }
    if (v == CPK_VALID) {
        uint64_t k0 = cpk_now_ns();
        Arena a;
        arena_init_detached(&a);
        CpkKernelCall call = {
            .plan = plan, .space = s, .index = p->index, .scratch = &a,
            .slot_vars = p->slot_vars, .factor_atoms = p->factor_atoms,
            .template_atom = p->template_atom, .inputs = inputs,
            .demand = p->opt.demand, .visitor = visit_answer, .user_data = c, .stats = &p->stats,
        };
        int rc = cpk_kernel_run(&call);
        if (rc == 0) {
            uint64_t m0 = cpk_now_ns();
            rc = materialize(c, &a);
            p->stats.answer_ns += cpk_now_ns() - m0;
        }
        c->nraw = 0;
        arena_free(&a);
        p->stats.kernel_ns += cpk_now_ns() - k0;
        if (rc == 0) { p->stats.kernel_calls++; return; }
        drop_answers(c);
        snprintf(p->last_reason, sizeof p->last_reason, "kernel aborted; fell back");
    }
    if (!residual(c, inputs)) { drop_answers(c); c->failed = true; }
}

static cetta_status_t cb_open(void *ctx, cetta_engine_t *engine, cetta_atom_t *const *args,
                              size_t arity, void **out) {
    Cursor *c = calloc(1, sizeof(*c));
    if (!c) return CETTA_OUT_OF_MEMORY;
    c->pack = ctx; c->engine = engine; c->args = args; c->nargs = arity;
    *out = c;
    return CETTA_OK;
}

static cetta_status_t cb_next(void *cursor, cetta_callback_state_t *state, cetta_atom_t **out) {
    Cursor *c = cursor;
    *out = NULL;
    if (!c->computed) { compute(c); c->computed = true; }
    if (c->failed) { *state = CETTA_CALLBACK_COMPLETE; return CETTA_ENGINE_ERROR; }
    if (c->pos < c->n) { *out = c->answers[c->pos++]; *state = CETTA_CALLBACK_VALUE; return CETTA_OK; }
    *state = CETTA_CALLBACK_COMPLETE;
    return CETTA_OK;
}

static void cb_close(void *cursor, cetta_callback_close_mode_t mode) {
    (void)mode;
    Cursor *c = cursor;
    for (size_t i = c->pos; i < c->n; i++) cetta_embed_atom_release(c->answers[i]);
    free(c->answers);
    free(c->raw);
    free(c);
}

static void ctx_retain(void *ctx) { (void)ctx; }
static void ctx_release(void *ctx) { (void)ctx; }

cetta_status_t cpk_pack_open(cetta_engine_t *engine, const CpkPackOptions *opt, CpkPack **out) {
    CpkPack *p = calloc(1, sizeof(*p));
    if (!p) return CETTA_OUT_OF_MEMORY;
    p->opt = *opt;
    p->engine = engine;
    p->loaded = pack_load(p, p->load_error, sizeof p->load_error);
    {
        const CpkJson *b = p->manifest ? cpk_json_get(p->manifest, "amortization.max_append_delta_atoms") : NULL;
        p->max_append_delta = b && b->kind == CPK_JSON_NUMBER && b->num >= 0 ? (uint64_t)b->num : 0;
    }
    cetta_callback_descriptor_t d = {
        .size = sizeof d, .arity = opt->plan->ninputs, .kind = CETTA_CALLBACK_STREAM,
        .context = p, .retain = ctx_retain, .release = ctx_release,
        .open = cb_open, .next = cb_next, .close = cb_close,
    };
    cetta_status_t st = cetta_embed_register_callback(engine, opt->plan->site, &d, &p->callable);
    if (st != CETTA_OK) { cpk_pack_close(p); return st; }
    *out = p;
    return CETTA_OK;
}

void cpk_pack_close(CpkPack *p) {
    if (!p) return;
    cpk_index_free(p->index);
    cetta_embed_atom_release(p->region_handle);
    cetta_embed_atom_release(p->callable);
    cpk_json_free(p->manifest);
    free(p);
}
