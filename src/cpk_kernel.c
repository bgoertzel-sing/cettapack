/* CettaPack M1 kernel runtime: PathMap narrowing + native seeded rematch.
 * See cpk.h for the contract. */
#define _POSIX_C_SOURCE 200809L
#include "cpk.h"

#include "match.h"
#include "mork_space_bridge_runtime.h"
#include "space_match_backend.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

uint64_t cpk_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

/* ---------------------------------------------------------------------- */
/* string -> id hash table (index build and support-mode dedup)           */

typedef struct {
    char **keys;
    uint64_t *vals;
    uint64_t cap, len;
} StrMap;

static uint64_t fnv1a(const char *s) {
    uint64_t h = 1469598103934665603ull;
    for (; *s; s++) { h ^= (unsigned char)*s; h *= 1099511628211ull; }
    return h;
}

static bool strmap_init(StrMap *m, uint64_t cap) {
    uint64_t c = 16;
    while (c < cap * 2) c <<= 1;
    m->keys = calloc(c, sizeof(char *));
    m->vals = calloc(c, sizeof(uint64_t));
    m->cap = c; m->len = 0;
    return m->keys && m->vals;
}

static void strmap_free(StrMap *m) {
    if (m->keys) for (uint64_t i = 0; i < m->cap; i++) free(m->keys[i]);
    free(m->keys); free(m->vals);
    memset(m, 0, sizeof(*m));
}

static bool strmap_grow(StrMap *m) {
    StrMap n;
    if (!strmap_init(&n, m->cap)) return false;
    for (uint64_t i = 0; i < m->cap; i++) {
        if (!m->keys[i]) continue;
        uint64_t j = fnv1a(m->keys[i]) & (n.cap - 1);
        while (n.keys[j]) j = (j + 1) & (n.cap - 1);
        n.keys[j] = m->keys[i]; n.vals[j] = m->vals[i]; n.len++;
    }
    free(m->keys); free(m->vals);
    *m = n;
    return true;
}

/* Returns the value for key, inserting next_val when absent. *inserted tells. */
static bool strmap_get_or_put(StrMap *m, const char *key, uint64_t next_val,
                              uint64_t *out, bool *inserted) {
    if ((m->len + 1) * 2 > m->cap && !strmap_grow(m)) return false;
    uint64_t j = fnv1a(key) & (m->cap - 1);
    while (m->keys[j]) {
        if (!strcmp(m->keys[j], key)) { *out = m->vals[j]; *inserted = false; return true; }
        j = (j + 1) & (m->cap - 1);
    }
    m->keys[j] = strdup(key);
    if (!m->keys[j]) return false;
    m->vals[j] = next_val; m->len++;
    *out = next_val; *inserted = true;
    return true;
}

/* ---------------------------------------------------------------------- */
/* narrowing index                                                         */

struct CpkIndex {
    Space *space;
    uint64_t revision, length;
    CettaMorkSpaceHandle *pm;     /* PathMap-backed MORK space; NULL if disabled */
    uint64_t ngroups, nrows;
    uint64_t *offsets;            /* CSR: group g owns rows[offsets[g] .. offsets[g+1]) */
    CettaIndex *rows;             /* source record positions in the borrowed space */
    bool schema_ok, pathmap_ok;
    bool direct_ok;               /* keys use MORK's inline symbol encoding: byte-prefix descent */
    AtomId *ids;                  /* the space's record ids at instantiation (revision policy) */
    char note[256];
};

bool cpk_index_schema_ok(const CpkIndex *ix) { return ix && ix->schema_ok; }
bool cpk_index_pathmap_ok(const CpkIndex *ix) { return ix && ix->pathmap_ok; }
Space *cpk_index_space(const CpkIndex *ix) { return ix ? ix->space : NULL; }
uint64_t cpk_index_revision(const CpkIndex *ix) { return ix ? ix->revision : 0; }
uint64_t cpk_index_length(const CpkIndex *ix) { return ix ? ix->length : 0; }
uint64_t cpk_index_groups(const CpkIndex *ix) { return ix ? ix->ngroups : 0; }
uint64_t cpk_index_rows(const CpkIndex *ix) { return ix ? ix->nrows : 0; }
const char *cpk_index_schema_note(const CpkIndex *ix) { return ix ? ix->note : "no index"; }
bool cpk_index_direct_ok(const CpkIndex *ix) { return ix && ix->direct_ok; }

/* append_only_epoch revision policy: the live space is an append of the
 * indexed one when it is at least as long and its first records are the
 * indexed records, in order. Read only on the slow path (revision changed). */
CpkChange cpk_index_change(const CpkIndex *ix, Space *s, uint64_t *delta) {
    uint64_t len = space_length64(s);
    *delta = 0;
    if (space_revision(s) == ix->revision && len == ix->length) return CPK_CHANGE_NONE;
    if (len < ix->length) return CPK_CHANGE_DESTRUCTIVE;
    for (uint64_t i = 0; i < ix->length; i++)
        if (space_get_atom_id_at64(s, (CettaIndex)i) != ix->ids[i]) return CPK_CHANGE_DESTRUCTIVE;
    *delta = len - ix->length;
    return *delta ? CPK_CHANGE_APPEND : CPK_CHANGE_DESTRUCTIVE;   /* same records, new revision: rewritten */
}

void cpk_index_free(CpkIndex *ix) {
    if (!ix) return;
    if (ix->pm) cetta_mork_bridge_space_free(ix->pm);
    free(ix->offsets); free(ix->rows); free(ix->ids);
    free(ix);
}

typedef struct { char *buf; size_t len, cap; } Buf;

static bool buf_put(Buf *b, const char *s, size_t n) {
    if (b->len + n + 1 > b->cap) {
        size_t c = b->cap ? b->cap : 4096;
        while (b->len + n + 1 > c) c *= 2;
        char *p = realloc(b->buf, c);
        if (!p) return false;
        b->buf = p; b->cap = c;
    }
    memcpy(b->buf + b->len, s, n);
    b->len += n; b->buf[b->len] = 0;
    return true;
}
static bool buf_puts(Buf *b, const char *s) { return buf_put(b, s, strlen(s)); }

/* Which factor (by head symbol and arity) a record may belong to. Records of
 * other relations are irrelevant to the site and stay unindexed. A record that
 * could unify with a factor but lies outside the compiled schema (a variable
 * record, a variable head, a non-symbol field, a wrong arity under a factor
 * head) makes the schema guard fail: the kernel would not be adequate. */
typedef enum { REC_IRRELEVANT, REC_INDEX, REC_VIOLATION } RecClass;

static RecClass classify(const CpkPlan *plan, const SymbolId *heads, Atom *a,
                         char *why, size_t why_len) {
    if (!a) { snprintf(why, why_len, "null record"); return REC_VIOLATION; }
    if (a->kind == ATOM_VAR) { snprintf(why, why_len, "variable record"); return REC_VIOLATION; }
    if (a->kind != ATOM_EXPR) return REC_IRRELEVANT;
    CettaExprLen n = a->expr.len;
    if (n == 0) return REC_IRRELEVANT;
    Atom *h = a->expr.elems[0];
    bool any_arity = false;
    for (uint8_t f = 0; f < plan->nfactors; f++)
        if (plan->factors[f].arity == n) any_arity = true;
    if (h->kind != ATOM_SYMBOL) {
        if (any_arity) { snprintf(why, why_len, "non-symbol head on arity-%" PRIu64 " record", (uint64_t)n); return REC_VIOLATION; }
        return REC_IRRELEVANT;
    }
    for (uint8_t f = 0; f < plan->nfactors; f++) {
        if (h->sym_id != heads[f]) continue;
        if (plan->factors[f].arity != n) {
            snprintf(why, why_len, "head %s with arity %" PRIu64, plan->factors[f].fields[0].text, (uint64_t)n);
            return REC_VIOLATION;
        }
        for (CettaExprLen i = 1; i < n; i++)
            if (a->expr.elems[i]->kind != ATOM_SYMBOL) {
                snprintf(why, why_len, "non-symbol field in a %s record", plan->factors[f].fields[0].text);
                return REC_VIOLATION;
            }
        return REC_INDEX;
    }
    return REC_IRRELEVANT;
}

/* MORK's expression bytes (MORK/expr, Tag): Arity(a) = a (a < 64);
 * SymbolSize(s) = 0xC0|s then s bytes (0 < s < 64); NewVar = 0xC0;
 * VarRef(r) = 0x80|r. A symbol is written inline when its text is plain. */
static bool plain_symbol_text(const char *t) {
    if (!t) return false;
    size_t n = strlen(t);
    if (n == 0 || n >= 64) return false;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)t[i];
        if (c <= 32 || c >= 127 || strchr("()\"$;\\", c)) return false;
    }
    return true;
}

static bool atom_symbols_inline(Arena *scratch, Atom *a) {
    if (a->kind == ATOM_SYMBOL) return plain_symbol_text(atom_to_parseable_string(scratch, a));
    if (a->kind != ATOM_EXPR || a->expr.len >= 64) return false;
    for (CettaExprLen i = 0; i < a->expr.len; i++)
        if (!atom_symbols_inline(scratch, a->expr.elems[i])) return false;
    return true;
}

/* skip one MORK expression starting at *pos */
static bool mork_skip(const uint8_t *b, size_t n, size_t *pos, int depth) {
    if (*pos >= n || depth > 64) return false;
    uint8_t t = b[(*pos)++];
    if ((t & 0xC0) == 0xC0) { if (t != 0xC0) *pos += (t & 0x3F); return *pos <= n; }
    if ((t & 0xC0) == 0x80) return true;
    if ((t & 0xC0) == 0x00) {
        for (uint8_t i = 0; i < t; i++) if (!mork_skip(b, n, pos, depth + 1)) return false;
        return true;
    }
    return false;
}

/* key = (<atom> <gid>): arity 2, the atom, then the gid symbol */
static bool key_gid(const uint8_t *b, size_t n, uint64_t *gid) {
    size_t pos = 1;
    if (n < 3 || b[0] != 0x02) return false;
    if (!mork_skip(b, n, &pos, 0) || pos >= n) return false;
    uint8_t t = b[pos++];
    if ((t & 0xC0) != 0xC0 || t == 0xC0) return false;
    size_t len = t & 0x3F;
    if (pos + len != n) return false;
    uint64_t g = 0;
    for (size_t i = 0; i < len; i++) {
        if (b[pos + i] < '0' || b[pos + i] > '9') return false;
        g = g * 10 + (uint64_t)(b[pos + i] - '0');
    }
    *gid = g;
    return true;
}

/* every stored key must decode to an in-range group id */
static bool direct_selfcheck(CpkIndex *ix) {
    CettaMorkCursorHandle *c = cetta_mork_bridge_cursor_new(ix->pm);
    if (!c) return false;
    bool moved = true, ok = true;
    uint64_t seen = 0;
    while (ok && cetta_mork_bridge_cursor_next_val(c, &moved) && moved) {
        uint8_t *b = NULL; size_t n = 0; uint64_t g = 0;
        ok = cetta_mork_bridge_cursor_path_bytes(c, &b, &n) && key_gid(b, n, &g) && g < ix->ngroups;
        if (b) cetta_mork_bridge_bytes_free(b, n);
        seen++;
    }
    cetta_mork_bridge_cursor_free(c);
    return ok && seen == ix->ngroups;
}

CpkIndex *cpk_index_build(const CpkPlan *plan, Space *space, Arena *scratch,
                          bool pathmap_enabled, char *why, size_t why_len) {
    CpkIndex *ix = calloc(1, sizeof(*ix));
    if (!ix) { snprintf(why, why_len, "out of memory"); return NULL; }
    ix->space = space;
    ix->revision = space_revision(space);
    ix->length = space_length64(space);
    ix->schema_ok = true;
    snprintf(ix->note, sizeof ix->note, "ok");

    SymbolId heads[CPK_MAX_FACTORS];
    for (uint8_t f = 0; f < plan->nfactors; f++)
        heads[f] = atom_symbol(scratch, plan->factors[f].fields[0].text)->sym_id;

    uint64_t len = ix->length;
    bool direct = true;
    uint64_t *gid_of_row = malloc(sizeof(uint64_t) * (len ? len : 1));
    StrMap groups;
    Buf text = {0};
    bool ok = gid_of_row && strmap_init(&groups, 1024);
    ix->ids = malloc(sizeof(AtomId) * (len ? len : 1));
    ok = ok && ix->ids;
    for (uint64_t i = 0; ok && i < len; i++) {
        ix->ids[i] = space_get_atom_id_at64(space, (CettaIndex)i);
        gid_of_row[i] = UINT64_MAX;
        Atom *a = space_get_at64(space, (CettaIndex)i);
        char note[160];
        RecClass c = classify(plan, heads, a, note, sizeof note);
        if (c == REC_VIOLATION) {
            if (ix->schema_ok)
                snprintf(ix->note, sizeof ix->note, "record %" PRIu64 ": %s", i, note);
            ix->schema_ok = false;
            continue;
        }
        if (c != REC_INDEX) continue;
        char *s = atom_to_parseable_string(scratch, a);
        if (!s) { ok = false; break; }
        if (direct && !atom_symbols_inline(scratch, a)) direct = false;
        uint64_t g; bool inserted;
        if (!strmap_get_or_put(&groups, s, ix->ngroups, &g, &inserted)) { ok = false; break; }
        if (inserted) {
            ix->ngroups++;
            char tail[32];
            snprintf(tail, sizeof tail, " %" PRIu64 ")\n", g);
            ok = buf_puts(&text, "(") && buf_puts(&text, s) && buf_puts(&text, tail);
        }
        gid_of_row[i] = g;
        ix->nrows++;
    }
    if (ok) {
        ix->offsets = calloc(ix->ngroups + 1, sizeof(uint64_t));
        ix->rows = malloc(sizeof(CettaIndex) * (ix->nrows ? ix->nrows : 1));
        ok = ix->offsets && ix->rows;
    }
    if (ok) {
        /* rows are kept in source order inside each group */
        for (uint64_t i = 0; i < len; i++)
            if (gid_of_row[i] != UINT64_MAX) ix->offsets[gid_of_row[i] + 1]++;
        for (uint64_t g = 0; g < ix->ngroups; g++) ix->offsets[g + 1] += ix->offsets[g];
        uint64_t *fill = calloc(ix->ngroups + 1, sizeof(uint64_t));
        ok = fill != NULL;
        for (uint64_t i = 0; ok && i < len; i++) {
            uint64_t g = gid_of_row[i];
            if (g == UINT64_MAX) continue;
            ix->rows[ix->offsets[g] + fill[g]++] = (CettaIndex)i;
        }
        free(fill);
    }
    if (ok && pathmap_enabled) {
        if (!cetta_mork_bridge_is_available()) {
            snprintf(ix->note, sizeof ix->note, "MORK/PathMap bridge unavailable");
        } else {
            ix->pm = cetta_mork_bridge_space_new();
            uint64_t added = 0;
            if (ix->pm && (ix->ngroups == 0 ||
                           cetta_mork_bridge_space_add_text(ix->pm, text.buf, &added))) {
                ix->pathmap_ok = true;
                ix->direct_ok = direct && direct_selfcheck(ix);
            }
            else
                snprintf(why, why_len, "bridge add_text failed: %s", cetta_mork_bridge_last_error());
        }
    }
    free(text.buf);
    free(gid_of_row);
    strmap_free(&groups);
    if (!ok) { snprintf(why, why_len, "index build failed (memory)"); cpk_index_free(ix); return NULL; }
    return ix;
}

/* ---------------------------------------------------------------------- */
/* narrowing: one MORK query over the PathMap index -> candidate group ids */

static bool rd32(const uint8_t *p, size_t len, size_t *off, uint32_t *v) {
    if (*off + 4 > len) return false;
    *v = ((uint32_t)p[*off] << 24) | ((uint32_t)p[*off + 1] << 16) |
         ((uint32_t)p[*off + 2] << 8) | (uint32_t)p[*off + 3];
    *off += 4; return true;
}
static bool rd64(const uint8_t *p, size_t len, size_t *off, uint64_t *v) {
    uint32_t hi, lo;
    if (!rd32(p, len, off, &hi) || !rd32(p, len, off, &lo)) return false;
    *v = ((uint64_t)hi << 32) | lo; return true;
}

typedef struct { uint64_t *items; uint64_t len, cap; } U64Vec;
static bool u64_push(U64Vec *v, uint64_t x) {
    if (v->len == v->cap) {
        uint64_t c = v->cap ? v->cap * 2 : 16;
        uint64_t *p = realloc(v->items, c * sizeof(uint64_t));
        if (!p) return false;
        v->items = p; v->cap = c;
    }
    v->items[v->len++] = x; return true;
}

/* query text: ((<factor fields>) $cpkgid); the group id variable is the
 * query-side variable whose first-occurrence index equals gid_index. */
static bool narrow(const CpkIndex *ix, const char *query, uint8_t gid_index,
                   U64Vec *out, CpkStats *st) {
    uint8_t *packet = NULL; size_t plen = 0; uint64_t rows = 0;
    uint64_t t0 = cpk_now_ns();
    bool ok = cetta_mork_bridge_space_query_bindings_text(ix->pm, query, &packet, &plen, &rows);
    if (ok) {
        size_t off = 0; uint64_t nrows = 0;
        ok = rd64(packet, plen, &off, &nrows);
        for (uint64_t r = 0; ok && r < nrows; r++) {
            uint32_t nref = 0, nb = 0;
            ok = rd32(packet, plen, &off, &nref);
            for (uint32_t k = 0; ok && k < nref; k++) { uint32_t dummy; ok = rd32(packet, plen, &off, &dummy); }
            ok = ok && rd32(packet, plen, &off, &nb);
            bool found = false;
            for (uint32_t k = 0; ok && k < nb; k++) {
                if (off + 2 > plen) { ok = false; break; }
                uint8_t side = packet[off], key = packet[off + 1];
                off += 2;
                uint32_t el = 0;
                ok = rd32(packet, plen, &off, &el) && off + el <= plen;
                if (!ok) break;
                if (side == 0 && key == gid_index) {
                    uint64_t g = 0; bool digits = el > 0;
                    for (uint32_t c = 0; c < el; c++) {
                        uint8_t ch = packet[off + c];
                        if (ch < '0' || ch > '9') { digits = false; break; }
                        g = g * 10 + (ch - '0');
                    }
                    if (!digits || g >= ix->ngroups) { ok = false; break; }
                    ok = u64_push(out, g);
                    found = true;
                }
                off += el;
            }
            if (ok && !found) ok = false;  /* every candidate row must name its group */
        }
    }
    if (packet) cetta_mork_bridge_bytes_free(packet, plen);
    st->narrow_ns += cpk_now_ns() - t0;
    st->narrow_queries++;
    return ok;
}

/* Direct narrowing: descend the PathMap trie on the key prefix (wrapper,
 * factor arity, then the ground fields up to the first unbound one) and read
 * the group id of every key below it. */
static bool narrow_direct(const CpkIndex *ix, CettaMorkCursorHandle *c,
                          const uint8_t *prefix, size_t plen, U64Vec *out, CpkStats *st) {
    uint64_t t0 = cpk_now_ns();
    bool ok = cetta_mork_bridge_cursor_reset(c), moved = true;
    for (size_t i = 0; ok && i < plen; i++) {
        ok = cetta_mork_bridge_cursor_descend_byte(c, prefix[i], &moved);
        if (ok && !moved) { st->narrow_ns += cpk_now_ns() - t0; st->narrow_queries++; return true; }
    }
    while (ok) {
        ok = cetta_mork_bridge_cursor_next_val(c, &moved);
        if (!ok || !moved) break;
        uint8_t *b = NULL; size_t n = 0; uint64_t g = 0;
        ok = cetta_mork_bridge_cursor_path_bytes(c, &b, &n);
        bool inside = ok && n >= plen && !memcmp(b, prefix, plen);
        if (inside) ok = key_gid(b, n, &g) && g < ix->ngroups && u64_push(out, g);
        if (b) cetta_mork_bridge_bytes_free(b, n);
        if (!inside) break;
    }
    st->narrow_ns += cpk_now_ns() - t0;
    st->narrow_queries++;
    return ok;
}

/* ---------------------------------------------------------------------- */
/* the join                                                                */

typedef struct {
    const CpkKernelCall *call;
    bool bound[CPK_MAX_FACTORS + 1][CPK_MAX_SLOTS];  /* slot bound before factor k */
    StrMap seen;                                      /* support demand only */
    CettaMorkCursorHandle *cursor;                    /* call-local PathMap cursor */
} JoinCtx;

static bool put_sym(uint8_t *buf, size_t cap, size_t *len, const char *t) {
    if (!plain_symbol_text(t)) return false;
    size_t n = strlen(t);
    if (*len + 1 + n > cap) return false;
    buf[(*len)++] = (uint8_t)(0xC0 | n);
    memcpy(buf + *len, t, n);
    *len += n;
    return true;
}

/* join/emit return 0 = continue, 1 = visitor said STOP, -1 = error */
static int join(JoinCtx *jc, uint8_t k, Bindings *env, uint64_t mult);

static int emit_answer(JoinCtx *jc, Bindings *env, uint64_t mult) {
    const CpkKernelCall *c = jc->call;
    uint64_t t0 = cpk_now_ns();
    Atom *answer = bindings_apply(env, c->scratch, c->template_atom);
    if (!answer) return -1;
    if (c->demand == CPK_DEMAND_SUPPORT) {
        /* support demand: one occurrence per distinct answer */
        char *s = atom_to_parseable_string(c->scratch, answer);
        uint64_t v; bool inserted;
        if (!s || !strmap_get_or_put(&jc->seen, s, 0, &v, &inserted)) return -1;
        if (!inserted) { c->stats->answer_ns += cpk_now_ns() - t0; return 0; }
        mult = 1;
    }
    CpkVisitCtl ctl = c->visitor(c->user_data, answer, mult);
    c->stats->visits++;
    c->stats->answers += mult;
    c->stats->answer_ns += cpk_now_ns() - t0;
    return ctl == CPK_VISIT_CONTINUE ? 0 : ctl == CPK_VISIT_STOP ? 1 : -1;
}

static int join(JoinCtx *jc, uint8_t k, Bindings *env, uint64_t mult) {
    const CpkKernelCall *c = jc->call;
    const CpkPlan *p = c->plan;
    if (k == p->nfactors) return emit_answer(jc, env, mult);
    const CpkFactor *f = &p->factors[k];

    /* Narrowing query: constants and bound slots are ground, so the trie
     * descends on them; unbound slots stay variables. Bindings read here are
     * used only to shape the query: answers come from rematch below. */
    Buf q = {0};
    bool ok = buf_puts(&q, "((");
    bool named[CPK_MAX_SLOTS] = {0};
    uint8_t nvars = 0;
    for (uint8_t i = 0; ok && i < f->arity; i++) {
        const CpkField *fd = &f->fields[i];
        if (i) ok = buf_puts(&q, " ");
        if (!ok) break;
        if (fd->kind == CPK_FIELD_CONST) { ok = buf_puts(&q, fd->text); continue; }
        if (jc->bound[k][fd->slot]) {
            Atom *v = bindings_apply(env, c->scratch, c->slot_vars[fd->slot]);
            if (!v || v->kind != ATOM_SYMBOL) { ok = false; break; }
            char *s = atom_to_parseable_string(c->scratch, v);
            ok = s && buf_puts(&q, s);
        } else {
            char name[24];
            snprintf(name, sizeof name, "$s%u", (unsigned)fd->slot);
            if (!named[fd->slot]) { named[fd->slot] = true; nvars++; }
            ok = buf_puts(&q, name);
        }
    }
    ok = ok && buf_puts(&q, ") $cpkgid)");
    U64Vec cands = {0};
    /* direct trie descent when the key prefix can be written byte for byte */
    uint8_t prefix[512];
    size_t plen = 0;
    bool direct = ok && jc->cursor != NULL;
    if (direct) {
        prefix[plen++] = 0x02;
        prefix[plen++] = f->arity;
        for (uint8_t i = 0; direct && i < f->arity; i++) {
            const CpkField *fd = &f->fields[i];
            if (fd->kind == CPK_FIELD_CONST) { direct = put_sym(prefix, sizeof prefix, &plen, fd->text); continue; }
            if (!jc->bound[k][fd->slot]) break;
            Atom *v = bindings_apply(env, c->scratch, c->slot_vars[fd->slot]);
            direct = v && v->kind == ATOM_SYMBOL &&
                     put_sym(prefix, sizeof prefix, &plen, atom_to_parseable_string(c->scratch, v));
        }
    }
    if (direct) ok = narrow_direct(c->index, jc->cursor, prefix, plen, &cands, c->stats);
    else if (ok) ok = narrow(c->index, q.buf, nvars, &cands, c->stats);
    free(q.buf);
    if (!ok) { free(cands.items); return -1; }
    c->stats->candidates += cands.len;

    int rc = 0;
    const CpkIndex *ix = c->index;
    for (uint64_t ci = 0; !rc && ci < cands.len; ci++) {
        uint64_t g = cands.items[ci];
        Bindings e;
        if (!bindings_clone(&e, env)) { rc = -1; break; }
        uint64_t t0 = cpk_now_ns();
        /* CeTTa's native seeded rematch on the candidate, read from its
         * first source record. Every record of a group is the same ground
         * all-symbol atom (schema guard), so the rematch result is the same
         * for each; the records only contribute their count. */
        bool m = space_match_backend_match_atom_seeded(
            c->space, ix->rows[ix->offsets[g]], c->factor_atoms[k], &e, c->scratch);
        c->stats->rematch_ns += cpk_now_ns() - t0;
        c->stats->rematches++;
        if (m) {
            uint64_t m0 = cpk_now_ns();
            /* §10.1: multiplicity from the source space records, composed
             * by product only after rematch confirmed compatibility */
            uint64_t count = ix->offsets[g + 1] - ix->offsets[g];
            uint64_t next = mult * count;
            c->stats->records += count;
            c->stats->mult_ns += cpk_now_ns() - m0;
            c->stats->rematch_ok++;
            if (count != 0 && next / count != mult) rc = -1;   /* overflow: fall back */
            else rc = join(jc, k + 1, &e, next);
        }
        bindings_free(&e);
    }
    free(cands.items);
    return rc;
}

int cpk_kernel_run(const CpkKernelCall *c) {
    if (!c || !c->plan || !c->space || !c->index || !c->index->pm || !c->visitor) return -1;
    JoinCtx jc;
    memset(&jc, 0, sizeof jc);
    jc.call = c;
    const CpkPlan *p = c->plan;
    for (uint8_t i = 0; i < p->ninputs; i++)
        if (p->input_slot[i] >= 0) jc.bound[0][p->input_slot[i]] = true;
    for (uint8_t k = 0; k < p->nfactors; k++) {
        memcpy(jc.bound[k + 1], jc.bound[k], sizeof jc.bound[k]);
        for (uint8_t i = 0; i < p->factors[k].arity; i++)
            if (p->factors[k].fields[i].kind == CPK_FIELD_SLOT)
                jc.bound[k + 1][p->factors[k].fields[i].slot] = true;
    }
    if (c->demand == CPK_DEMAND_SUPPORT && !strmap_init(&jc.seen, 64)) return -1;
    if (c->index->direct_ok) jc.cursor = cetta_mork_bridge_cursor_new(c->index->pm);

    Bindings env;
    bindings_init(&env);
    int rc = 0;
    for (uint8_t i = 0; !rc && i < p->ninputs; i++) {
        int8_t s = p->input_slot[i];
        if (s < 0) continue;
        if (!bindings_add_var(&env, c->slot_vars[s], c->inputs[i])) rc = -1;
    }
    if (!rc) rc = join(&jc, 0, &env, 1);
    if (rc == 1) rc = 0;   /* the visitor stopped the stream: a complete run */
    bindings_free(&env);
    if (c->demand == CPK_DEMAND_SUPPORT) strmap_free(&jc.seen);
    if (jc.cursor) cetta_mork_bridge_cursor_free(jc.cursor);
    return rc;
}
