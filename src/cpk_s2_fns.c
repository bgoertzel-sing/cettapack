/* cettapack S2 (t-2026-11-24-cettapack-s2-agenda-functions): PeTTaChainer functions compiled whole, inside CeTTa's
 * PeTTa runs.
 *
 * Built into CeTTa as src/cetta_s2_fns.c (bench/s2/build_s2_cetta.sh copies this file and src/cpk_s2_fns_text.h over
 * the stub of upstream/cetta-s2/0008), with `make CETTA_S2_FNS=1`.  The interface is src/cetta_s2_fns.h of 0008: when
 * PeTTa's search machine dispatches a call whose operands are ready (values), on a head a compiled function claims,
 * before equation selection, it offers the call (operands with the machine's bindings applied) to cetta_s2_fns_try; a
 * compiled function whose guards hold returns the call's single answer, which the machine unifies with the goal's
 * expected result; otherwise it declines, having changed nothing, and the machine runs the call itself (the residual).
 *
 * What is compiled (PeTTaChainer b0e24f9b; texts in src/cpk_s2_fns_text.h, tools/s2_fngen.py):
 *   frontier-remaining-score, with its callees and-bundle-premises, premises-expected-confidence(-step),
 *     term-confidence, proof-score, tv-confidence, frontier-pool-potential, expr-arity;
 *   make-and-bundle;
 *   proof-output-key-by-id, with proof-output-atom-by-id, proof-output-atom, candidate-output-key.
 * Each of these is also an entry point of its own.  All are deterministic and write nothing: they read &self's
 * definitions and (proof-output-atom) the &proof space.  A compiled function either computes the answer CeTTa computes
 * or declines the whole call; since nothing was written, declining at any point is the residual.
 *
 * Guards (fnspec's, per function, docs/S2_AGENDA_FUNCTIONS.md):
 *   - definition, per program revision of every space the function reads (&self, whose program token covers the
 *     imported modules' atoms and an overlay's base chain, and the data spaces it reads, here &proof): in &self, the equations of the function and of every function its code
 *     reproduces are exactly the compiled ones, alpha-equal and in order, with the same type declaration; no
 *     catch-all equation; no equation for any constructor the code builds or dispatches on; in every other space it
 *     reads, no equation for any of those heads nor a catch-all (a definition placed there takes the residual).
 *     Revalidated when space_program_token (equation/declaration revision) of any of those spaces moves.
 *   - builtins: no equation in &self for any builtin the code reproduces (if, case, let, let*, ==, <, +, *, min, max,
 *     is-var, cdr-atom, copy_term, once, match, quote, cons, collapse).
 *   - per call: the operands are values of the shapes the code covers.  Where CeTTa would bind a variable of the call
 *     (unification against a pattern, e.g. case on an unbound head), raise an error, or run code outside the compiled
 *     set (a CPU premise's formula, a non-numeric operand of min/max/<), the call declines.  Numbers go through
 *     CeTTa's own scalar operations (petta_numeric_min_max, grounded_try_plain_scalar_*).
 *   - proof-output-atom: the &proof query must give exactly one output up to renaming (pids are keys; `once` takes
 *     the matcher's first row, whose order the compiled query does not reproduce); it is answered as stored, renamed
 *     apart (fresh variables), as CeTTa's match answers it (CeTTa does not evaluate any part of it).
 *   - $CETTA_S2_FNS=invalid forces every guard invalid (every call takes the residual; tests).
 * Counters per function go to $CETTA_S2_FNS_STATS (JSON) at exit; $CETTA_S2_FNS_TRACE=1 prints declines to stderr.
 */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "cetta_s2_fns.h"
#if CETTA_S2_FNS
#include "atom.h"
#include "grounded.h"
#include "match.h"
#include "parser.h"
#include "petta_semantics.h"
#include "space.h"
#include "symbol.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpk_s2_fns_text.h"

/* ---- the function table -------------------------------------------------------------------------------- */

enum {
    F_MAKE_AND_BUNDLE,
    F_AND_BUNDLE_PREMISES,
    F_FRONTIER_REMAINING_SCORE,
    F_FRONTIER_POOL_POTENTIAL,
    F_EXPR_ARITY,
    F_PREMISES_EXPECTED_CONFIDENCE,
    F_PREMISES_EXPECTED_CONFIDENCE_STEP,
    F_TERM_CONFIDENCE,
    F_PROOF_SCORE,
    F_TV_CONFIDENCE,
    F_PROOF_OUTPUT_KEY_BY_ID,
    F_PROOF_OUTPUT_ATOM_BY_ID,
    F_PROOF_OUTPUT_ATOM,
    F_CANDIDATE_OUTPUT_KEY,
    F_N
};

typedef struct {
    const char *name;
    unsigned arity;
    const char *const *eqs; /* verbatim equations */
    const char *decl;       /* verbatim type declaration */
    bool entry;             /* offered as a call of its own */
    int deps[12];           /* functions whose equations its code reproduces (closed under calls), -1 ends */
    const char *spaces[4];  /* data spaces it reads (besides &self's definitions) */
} S2FnDef;

#define D(x) S2_DEF_##x##_EQS, S2_DEF_##x##_DECL
static const S2FnDef g_defs[F_N] = {
    [F_MAKE_AND_BUNDLE] = {"make-and-bundle", 2, D(MAKE_AND_BUNDLE), true, {-1}, {NULL}},
    [F_AND_BUNDLE_PREMISES] = {"and-bundle-premises", 1, D(AND_BUNDLE_PREMISES), true, {-1}, {NULL}},
    [F_FRONTIER_REMAINING_SCORE] = {"frontier-remaining-score", 2, D(FRONTIER_REMAINING_SCORE), true,
        {F_AND_BUNDLE_PREMISES, F_PREMISES_EXPECTED_CONFIDENCE, F_PREMISES_EXPECTED_CONFIDENCE_STEP,
         F_TERM_CONFIDENCE, F_PROOF_SCORE, F_TV_CONFIDENCE, F_FRONTIER_POOL_POTENTIAL, F_EXPR_ARITY, -1}, {NULL}},
    [F_FRONTIER_POOL_POTENTIAL] = {"frontier-pool-potential", 1, D(FRONTIER_POOL_POTENTIAL), true,
        {F_EXPR_ARITY, F_TV_CONFIDENCE, -1}, {NULL}},
    [F_EXPR_ARITY] = {"expr-arity", 1, D(EXPR_ARITY), true, {-1}, {NULL}},
    [F_PREMISES_EXPECTED_CONFIDENCE] = {"premises-expected-confidence", 1, D(PREMISES_EXPECTED_CONFIDENCE), true,
        {F_PREMISES_EXPECTED_CONFIDENCE_STEP, F_TERM_CONFIDENCE, F_PROOF_SCORE, F_TV_CONFIDENCE, -1}, {NULL}},
    [F_PREMISES_EXPECTED_CONFIDENCE_STEP] = {"premises-expected-confidence-step", 2,
        D(PREMISES_EXPECTED_CONFIDENCE_STEP), true,
        {F_PREMISES_EXPECTED_CONFIDENCE, F_TERM_CONFIDENCE, F_PROOF_SCORE, F_TV_CONFIDENCE, -1}, {NULL}},
    [F_TERM_CONFIDENCE] = {"term-confidence", 1, D(TERM_CONFIDENCE), true, {F_PROOF_SCORE, F_TV_CONFIDENCE, -1},
        {NULL}},
    [F_PROOF_SCORE] = {"proof-score", 2, D(PROOF_SCORE), true, {F_TV_CONFIDENCE, -1}, {NULL}},
    [F_TV_CONFIDENCE] = {"tv-confidence", 1, D(TV_CONFIDENCE), true, {-1}, {NULL}},
    [F_PROOF_OUTPUT_KEY_BY_ID] = {"proof-output-key-by-id", 1, D(PROOF_OUTPUT_KEY_BY_ID), true,
        {F_PROOF_OUTPUT_ATOM_BY_ID, F_PROOF_OUTPUT_ATOM, F_CANDIDATE_OUTPUT_KEY, -1}, {"&proof", NULL}},
    [F_PROOF_OUTPUT_ATOM_BY_ID] = {"proof-output-atom-by-id", 1, D(PROOF_OUTPUT_ATOM_BY_ID), true,
        {F_PROOF_OUTPUT_ATOM, -1}, {"&proof", NULL}},
    [F_PROOF_OUTPUT_ATOM] = {"proof-output-atom", 1, D(PROOF_OUTPUT_ATOM), false, {-1}, {"&proof", NULL}},
    [F_CANDIDATE_OUTPUT_KEY] = {"candidate-output-key", 1, D(CANDIDATE_OUTPUT_KEY), true, {-1}, {NULL}},
};
#undef D

/* constructors the code builds or dispatches on: no equations allowed; declarations as compiled */
static const struct { const char *name; const char *decl; } g_ctors[] = {
    {"and-bundle", S2_DEF_AND_BUNDLE_DECL}, {"proof-output-key", S2_DEF_PROOF_OUTPUT_KEY_DECL},
    {"STV", S2_DEF_STV_DECL}, {"CTV", S2_DEF_CTV_DECL}, {"CPU", S2_DEF_CPU_DECL},
    {"pool-part", S2_DEF_POOL_PART_DECL}, {"marginal-proj", S2_DEF_MARGINAL_PROJ_DECL},
    {"stored-proof-row", S2_DEF_STORED_PROOF_ROW_DECL},
};
#define S2_NCTORS (sizeof(g_ctors) / sizeof(g_ctors[0]))
static const char *const g_builtins[] = {"if", "case", "let", "let*", "==", "<", "+", "*", "min", "max", "is-var",
    "cdr-atom", "copy_term", "once", "match", "quote", "cons", "collapse", NULL};

/* ---- run-time state ------------------------------------------------------------------------------------ */

typedef struct {
    uint64_t offered, answered, declined_invalid, declined_forced, declined_call;
    uint64_t inner_calls; /* calls of this function made by compiled code (not offered by the machine) */
} S2FnStats;

static S2FnStats g_st[F_N];
static uint64_t g_revalidations, g_offers_unclaimed;
#define S2_NREASONS 32
static struct { const char *why; uint64_t n; } g_reasons[S2_NREASONS];

static int g_mode = -1;  /* 0 off, 1 on, 2 forced invalid */
static int g_trace = 0;
static Arena g_arena;
static bool g_init, g_init_failed;
static SymbolId g_sym[F_N];
static SymbolId g_ctor_sym[S2_NCTORS];
static Atom *g_ctor_decl[S2_NCTORS], *g_ctor_decl_query[S2_NCTORS];
static SymbolId g_builtin_sym[32];
static Atom *g_eq_atoms[F_N][4];
static unsigned g_eq_count[F_N];
static Atom *g_decl_atom[F_N];
static Atom *g_eq_query[F_N], *g_decl_query[F_N];
static SymbolId s_and_bundle, s_cpu, s_stv, s_ctv, s_pool_part, s_marginal_proj, s_proof_output_key, s_quote,
    s_stored_proof_row, s_cons, s_proof_space, s_plus, s_times, s_lt;

/* validity: one snapshot of the program tokens of every space read */
#define S2_MAX_SPACES 64
static struct {
    bool have;
    SpaceProgramToken self_tok;
    Space *spaces[S2_MAX_SPACES];
    SpaceProgramToken toks[S2_MAX_SPACES];
    unsigned nspaces;
    uint32_t ndeps;
    bool valid[F_N];
    const char *reason[F_N];
} g_val;

static void s2_note(const char *why) {
    for (unsigned i = 0; i < S2_NREASONS; i++) {
        if (g_reasons[i].why == why || (g_reasons[i].why && !strcmp(g_reasons[i].why, why))) {
            g_reasons[i].n++;
            return;
        }
        if (!g_reasons[i].why) {
            g_reasons[i].why = why;
            g_reasons[i].n = 1;
            return;
        }
    }
}

/* ---- evaluation context -------------------------------------------------------------------------------- */

typedef struct {
    const CettaS2FnCall *call;
    Arena *a;
    const char *decline;
    unsigned depth;
} S2Ctx;

#define S2_DECLINE(c, why)                                                                                    \
    do {                                                                                                     \
        if (!(c)->decline)                                                                                    \
            (c)->decline = (why);                                                                             \
        return NULL;                                                                                          \
    } while (0)

/* A value as PeTTa observes it: list carriers reified as the flat expressions they stand for
 * (petta_semantics_materialize_value, CeTTa's own observation). */
static Atom *s2_observe(Arena *a, Atom *x) {
    if (!x || !petta_semantics_value_contains_observable_open_cons(x))
        return x;
    return petta_semantics_materialize_value(a, x);
}

static bool s2_is_sym(const Atom *x, SymbolId s) { return x && x->kind == ATOM_SYMBOL && x->sym_id == s; }
static bool s2_is_num(const Atom *x) {
    return x && x->kind == ATOM_GROUNDED && (x->ground.gkind == GV_INT || x->ground.gkind == GV_FLOAT);
}

/* The head of an expression a pattern with a constructor head `ctor` and `len` elements is unified with: 1 the
 * pattern unifies structurally at the root (head is ctor, length is len), 0 it cannot, -1 unification would bind a
 * variable of the value at the root (the value or its head is a variable): the call declines. */
static int s2_root(const Atom *x, SymbolId ctor, CettaExprLen len) {
    if (!x)
        return -1;
    if (x->kind == ATOM_VAR)
        return -1;
    if (x->kind != ATOM_EXPR || x->expr.len != len)
        return 0;
    const Atom *h = x->expr.elems[0];
    if (h->kind == ATOM_VAR)
        return -1;
    return s2_is_sym(h, ctor) ? 1 : 0;
}

/* A list (cons view): 1 with head/tail of a non-empty list, 0 for (), -1 anything else (decline). */
static int s2_list(S2Ctx *c, Atom *x, Atom **head, Atom **tail) {
    if (!x || x->kind != ATOM_EXPR)
        return -1;
    if (x->expr.len == 0)
        return 0;
    if (s2_is_sym(x->expr.elems[0], s_cons))
        return -1; /* a literal (cons ...) in data: not a case the code covers */
    *head = x->expr.elems[0];
    *tail = x->expr.len == 1 ? atom_unit(c->a) : atom_expr(c->a, x->expr.elems + 1, x->expr.len - 1);
    return *tail ? 1 : -1;
}

static Atom *s2_minmax(S2Ctx *c, bool minimum, Atom *x, Atom *y) {
    Atom *out = NULL;
    if (!s2_is_num(x) || !s2_is_num(y))
        S2_DECLINE(c, "min/max on a non-number");
    if (!petta_numeric_min_max(c->a, minimum, x, y, &out) || !out)
        S2_DECLINE(c, "min/max outside the scalar fast path");
    return out;
}

static Atom *s2_arith(S2Ctx *c, SymbolId op, Atom *x, Atom *y) {
    if (!s2_is_num(x) || !s2_is_num(y))
        S2_DECLINE(c, "arithmetic on a non-number");
    Atom *head = atom_symbol_id(c->a, op);
    Atom *args[2] = {x, y};
    Atom *out = NULL;
    if (!head || !grounded_try_plain_scalar_arithmetic(c->a, head, args, 2u, &out) || !out)
        S2_DECLINE(c, "arithmetic outside the scalar fast path");
    return out;
}

static int s2_less(S2Ctx *c, Atom *x, Atom *y) {
    if (!s2_is_num(x) || !s2_is_num(y)) {
        c->decline = c->decline ? c->decline : "< on a non-number";
        return -1;
    }
    Atom *head = atom_symbol_id(c->a, s_lt);
    Atom *args[2] = {x, y};
    bool t = false;
    if (!head || !grounded_try_plain_scalar_truth(head, args, 2u, &t)) {
        c->decline = c->decline ? c->decline : "< outside the scalar fast path";
        return -1;
    }
    return t ? 1 : 0;
}

/* ---- the compiled functions ---------------------------------------------------------------------------- */
/* Each takes observed values and returns the value CeTTa's evaluation of the call gives, or NULL (decline). */

static Atom *fn_tv_confidence(S2Ctx *c, Atom *tv);

/* (= (tv-confidence $tv)
 *    (if (is-var $tv) 1.0
 *       (case $tv (((STV $_s $c) (if (is-var $c) 1.0 $c))
 *                  ((CTV $pos $neg) (min (tv-confidence $pos) (tv-confidence $neg)))
 *                  ($_other 1.0))))) */
static Atom *fn_tv_confidence(S2Ctx *c, Atom *tv) {
    if (++c->depth > 256)
        S2_DECLINE(c, "depth");
    Atom *r;
    if (tv->kind == ATOM_VAR) {
        r = atom_float(c->a, 1.0);
    } else {
        int m = s2_root(tv, s_stv, 3);
        if (m < 0)
            S2_DECLINE(c, "tv-confidence: case would bind a variable");
        if (m == 1) {
            Atom *cf = tv->expr.elems[2];
            r = cf->kind == ATOM_VAR ? atom_float(c->a, 1.0) : cf;
        } else {
            m = s2_root(tv, s_ctv, 3);
            if (m < 0)
                S2_DECLINE(c, "tv-confidence: case would bind a variable");
            if (m == 1) {
                Atom *p = fn_tv_confidence(c, tv->expr.elems[1]);
                if (!p)
                    return NULL;
                Atom *n = fn_tv_confidence(c, tv->expr.elems[2]);
                if (!n)
                    return NULL;
                r = s2_minmax(c, true, p, n);
            } else {
                r = atom_float(c->a, 1.0);
            }
        }
    }
    c->depth--;
    return r;
}

/* (= (proof-score $prf $tv)
 *    (case $prf (((marginal-proj $_idx $_inner) (* 0.99 (tv-confidence $tv)))
 *                ($_other (tv-confidence $tv))))) */
static Atom *fn_proof_score(S2Ctx *c, Atom *prf, Atom *tv) {
    int m = s2_root(prf, s_marginal_proj, 3);
    if (m < 0)
        S2_DECLINE(c, "proof-score: case would bind a variable");
    Atom *t = fn_tv_confidence(c, tv);
    if (!t)
        return NULL;
    if (m == 1)
        return s2_arith(c, s_times, atom_float(c->a, 0.99), t);
    return t;
}

/* (= (term-confidence $proof-term)
 *    (case $proof-term
 *       (((CPU $fun $args $res) ...)                                    -> declined (formulas: not compiled)
 *        (($_type ($_kbid $_ctx $_vars) $prf $tv)
 *           (if (is-var $prf) (* 0.99 (tv-confidence $tv)) (proof-score $prf $tv)))
 *        ($_other 0.0)))) */
static Atom *fn_term_confidence(S2Ctx *c, Atom *t) {
    int m = s2_root(t, s_cpu, 4);
    if (m < 0)
        S2_DECLINE(c, "term-confidence: case would bind a variable");
    if (m == 1)
        S2_DECLINE(c, "term-confidence: CPU term (formula evaluation not compiled)");
    if (t->kind == ATOM_EXPR && t->expr.len == 4) {
        Atom *ctx = t->expr.elems[1];
        if (ctx->kind == ATOM_VAR)
            S2_DECLINE(c, "term-confidence: case would bind a variable");
        if (ctx->kind == ATOM_EXPR && ctx->expr.len == 3) {
            Atom *prf = t->expr.elems[2], *tv = t->expr.elems[3];
            if (prf->kind == ATOM_VAR) {
                Atom *x = fn_tv_confidence(c, tv);
                return x ? s2_arith(c, s_times, atom_float(c->a, 0.99), x) : NULL;
            }
            return fn_proof_score(c, prf, tv);
        }
    }
    return atom_float(c->a, 0.0);
}

static Atom *fn_premises_expected_confidence(S2Ctx *c, Atom *ps);

/* (= (premises-expected-confidence-step $prem $rest)
 *    (case $prem (((CPU $fun $args $res) ...)                         -> declined (formulas: not compiled)
 *                 ($other (min (term-confidence $other) (premises-expected-confidence $rest)))))) */
static Atom *fn_premises_expected_confidence_step(S2Ctx *c, Atom *prem, Atom *rest) {
    int m = s2_root(prem, s_cpu, 4);
    if (m < 0)
        S2_DECLINE(c, "premises-expected-confidence-step: case would bind a variable");
    if (m == 1)
        S2_DECLINE(c, "premises-expected-confidence-step: CPU premise (formula evaluation not compiled)");
    Atom *x = fn_term_confidence(c, prem);
    if (!x)
        return NULL;
    Atom *y = fn_premises_expected_confidence(c, rest);
    if (!y)
        return NULL;
    return s2_minmax(c, true, x, y);
}

/* (= (premises-expected-confidence ()) 1.0)
 * (= (premises-expected-confidence (cons $prem $rest)) (premises-expected-confidence-step $prem $rest)) */
static Atom *fn_premises_expected_confidence(S2Ctx *c, Atom *ps) {
    if (++c->depth > 256)
        S2_DECLINE(c, "depth");
    Atom *h = NULL, *t = NULL;
    int k = s2_list(c, ps, &h, &t);
    Atom *r;
    if (k < 0)
        S2_DECLINE(c, "premises-expected-confidence: not a list");
    if (k == 0)
        r = atom_float(c->a, 1.0);
    else
        r = fn_premises_expected_confidence_step(c, h, t);
    if (r)
        c->depth--;
    return r;
}

/* (= (expr-arity $e) (if (== $e ()) 0 (+ 1 (expr-arity (cdr-atom $e))))) */
static Atom *fn_expr_arity(S2Ctx *c, Atom *e) {
    if (!e || e->kind != ATOM_EXPR)
        S2_DECLINE(c, "expr-arity: not an expression");
    if (e->expr.len > 0 && s2_is_sym(e->expr.elems[0], s_cons))
        S2_DECLINE(c, "expr-arity: a literal (cons ...)");
    return atom_int(c->a, (int64_t)e->expr.len);
}

/* (= (frontier-pool-potential ()) 0.0)
 * (= (frontier-pool-potential (cons $part $rest))
 *    (case $part (((pool-part $_k $btv $n $idxs $_ev $_mtv)
 *                    (if (< (expr-arity $idxs) $n)
 *                        (max (tv-confidence $btv) (frontier-pool-potential $rest))
 *                        (frontier-pool-potential $rest)))
 *                 ($_ (frontier-pool-potential $rest))))) */
static Atom *fn_frontier_pool_potential(S2Ctx *c, Atom *parts) {
    if (++c->depth > 256)
        S2_DECLINE(c, "depth");
    Atom *part = NULL, *rest = NULL;
    int k = s2_list(c, parts, &part, &rest);
    Atom *r;
    if (k < 0)
        S2_DECLINE(c, "frontier-pool-potential: not a list");
    if (k == 0) {
        r = atom_float(c->a, 0.0);
    } else {
        int m = s2_root(part, s_pool_part, 7);
        if (m < 0)
            S2_DECLINE(c, "frontier-pool-potential: case would bind a variable");
        if (m == 1) {
            Atom *ar = fn_expr_arity(c, part->expr.elems[4]);
            if (!ar)
                return NULL;
            int lt = s2_less(c, ar, part->expr.elems[3]);
            if (lt < 0)
                return NULL;
            if (lt) {
                Atom *x = fn_tv_confidence(c, part->expr.elems[2]);
                if (!x)
                    return NULL;
                Atom *y = fn_frontier_pool_potential(c, rest);
                if (!y)
                    return NULL;
                r = s2_minmax(c, false, x, y);
            } else {
                r = fn_frontier_pool_potential(c, rest);
            }
        } else {
            r = fn_frontier_pool_potential(c, rest);
        }
    }
    if (r)
        c->depth--;
    return r;
}

/* (= (and-bundle-premises (and-bundle $_goal $premises)) $premises) */
static Atom *fn_and_bundle_premises(S2Ctx *c, Atom *b) {
    int m = s2_root(b, s_and_bundle, 3);
    if (m < 0)
        S2_DECLINE(c, "and-bundle-premises: head unification would bind a variable");
    if (m == 0)
        S2_DECLINE(c, "and-bundle-premises: no equation applies");
    return b->expr.elems[2];
}

/* (= (frontier-remaining-score $bundle $parts)
 *    (max (premises-expected-confidence (copy_term (and-bundle-premises $bundle)))
 *         (frontier-pool-potential $parts)))
 * copy_term only protects the caller's variables from the bindings premises-expected-confidence's unifications
 * make; the compiled code makes none (it declines where CeTTa would bind), and the copy's variables cannot reach
 * the answer (a number), so the copy is not built. */
static Atom *fn_frontier_remaining_score(S2Ctx *c, Atom *bundle, Atom *parts) {
    Atom *ps = fn_and_bundle_premises(c, bundle);
    if (!ps)
        return NULL;
    Atom *x = fn_premises_expected_confidence(c, ps);
    if (!x)
        return NULL;
    Atom *y = fn_frontier_pool_potential(c, parts);
    if (!y)
        return NULL;
    return s2_minmax(c, false, x, y);
}

/* (= (make-and-bundle $goal $args) (and-bundle $goal $args)) */
static Atom *fn_make_and_bundle(S2Ctx *c, Atom *goal, Atom *args) {
    Atom *e[3] = {atom_symbol_id(c->a, s_and_bundle), goal, args};
    Atom *r = e[0] ? atom_expr(c->a, e, 3) : NULL;
    if (!r)
        S2_DECLINE(c, "allocation");
    return r;
}

typedef struct { VarId a[512], b[512]; unsigned n; } S2VarMap;
static bool s2_alpha(const Atom *x, const Atom *y, S2VarMap *m);

/* x with every variable replaced by a fresh one (same spelling), consistently: the renamed-apart copy of a stored
 * atom that a match answers.  NULL on allocation failure or more than 512 distinct variables. */
static Atom *s2_freshen(Arena *a, Atom *x, S2VarMap *m) {
    if (!x || !atom_has_vars(x))
        return x;
    if (x->kind == ATOM_VAR) {
        for (unsigned i = 0; i < m->n; i++)
            if (var_id_eq(m->a[i], x->var_id))
                return atom_var_like(a, x, m->b[i]);
        if (m->n == 512)
            return NULL;
        m->a[m->n] = x->var_id;
        m->b[m->n] = fresh_var_id();
        return atom_var_like(a, x, m->b[m->n++]);
    }
    if (x->kind != ATOM_EXPR)
        return x;
    Atom *el[x->expr.len ? x->expr.len : 1];
    for (CettaExprLen i = 0; i < x->expr.len; i++)
        if (!(el[i] = s2_freshen(a, x->expr.elems[i], m)))
            return NULL;
    return atom_expr(a, el, x->expr.len);
}

/* (= (proof-output-atom $pid)
 *    (match &proof (stored-proof-row $pid $_gid $output $_kind $_score $_ev $_children) $output))
 * (= (proof-output-atom-by-id $pid) (once (proof-output-atom $pid)))
 * The rows of &proof for $pid; the code answers only when they give exactly one distinct output (so `once`'s choice
 * of the first row does not matter), a ground value. */
static Atom *fn_proof_output_atom_by_id(S2Ctx *c, Atom *pid) {
    if (!pid || pid->kind != ATOM_GROUNDED || pid->ground.gkind != GV_INT)
        S2_DECLINE(c, "proof-output-atom-by-id: pid not an integer");
    Atom *ref = atom_symbol_id(c->a, s_proof_space);
    Space *sp = ref ? c->call->resolve(c->call->ctx, ref) : NULL;
    if (!sp)
        S2_DECLINE(c, "proof-output-atom-by-id: &proof not resolved");
    /* the pattern (stored-proof-row pid $g $o $k $s $e $c) */
    Atom *el[8];
    el[0] = atom_symbol_id(c->a, s_stored_proof_row);
    el[1] = pid;
    static const char *vn[6] = {"cpk_s2_g", "cpk_s2_o", "cpk_s2_k", "cpk_s2_s", "cpk_s2_e", "cpk_s2_c"};
    Atom *out_var = NULL;
    for (unsigned i = 0; i < 6; i++) {
        el[2 + i] = atom_var_with_id(c->a, vn[i], fresh_var_id());
        if (i == 1)
            out_var = el[2 + i];
    }
    Atom *pat = atom_expr(c->a, el, 8);
    if (!pat || !out_var)
        S2_DECLINE(c, "allocation");
    Bindings seed;
    bindings_init(&seed);
    BindingSet rows;
    Atom *q = pat;
    space_query_conjunction(sp, c->a, &q, 1, &seed, &rows);
    Atom *answer = NULL;
    const char *why = NULL;
    if (rows.len == 0)
        why = "proof-output-atom-by-id: no row (CeTTa's answer is no result)";
    for (CettaCount i = 0; i < rows.len && !why; i++) {
        Atom *o = bindings_apply(&rows.items[i], c->a, out_var);
        if (!o) {
            why = "allocation";
            break;
        }
        o = s2_observe(c->a, o);
        S2VarMap m = {.n = 0};
        if (!answer)
            answer = o;
        else if (!s2_alpha(answer, o, &m))
            why = "proof-output-atom-by-id: more than one distinct output (row order)";
    }
    binding_set_free(&rows);
    bindings_free(&seed);
    if (why)
        S2_DECLINE(c, why);
    /* The output is answered as stored (renamed apart): CeTTa returns match's $output as the row holds it, without
     * evaluating any part of it, even parts with equations (observed, evidence/s2-2026-11-26/probes/nf; bag test
     * t2 checks it per mode), so no normal-form check applies here. */
    S2VarMap fm = {.n = 0};
    Atom *fresh = s2_freshen(c->a, answer, &fm);
    if (!fresh)
        S2_DECLINE(c, "proof-output-atom-by-id: renaming the output apart failed");
    return fresh;
}

/* (= (candidate-output-key $term)
 *    (case $term ((($type $kb $_prf $_tv) (proof-output-key (quote ($type $kb))))
 *                 ($_other (proof-output-key $term)))))
 * In CeTTa's PeTTa, (quote X) as an evaluated argument gives X itself: the key CeTTa builds is
 * (proof-output-key ($type $kb)), with no quote (observed: bag test t1 and evidence/s2-2026-11-26/probes). */
static Atom *fn_candidate_output_key(S2Ctx *c, Atom *t) {
    if (!t || t->kind == ATOM_VAR)
        S2_DECLINE(c, "candidate-output-key: case would bind a variable");
    Atom *key;
    if (t->kind == ATOM_EXPR && t->expr.len == 4) {
        Atom *pair[2] = {t->expr.elems[0], t->expr.elems[1]};
        key = atom_expr(c->a, pair, 2);
    } else {
        key = t;
    }
    Atom *ke[2] = {atom_symbol_id(c->a, s_proof_output_key), key};
    Atom *r = key && ke[0] ? atom_expr(c->a, ke, 2) : NULL;
    if (!r)
        S2_DECLINE(c, "allocation");
    return r;
}

/* (= (proof-output-key-by-id $pid) (candidate-output-key (proof-output-atom-by-id $pid))) */
static Atom *fn_proof_output_key_by_id(S2Ctx *c, Atom *pid) {
    Atom *o = fn_proof_output_atom_by_id(c, pid);
    return o ? fn_candidate_output_key(c, o) : NULL;
}

/* ---- guards -------------------------------------------------------------------------------------------- */

static bool s2_alpha(const Atom *x, const Atom *y, S2VarMap *m) {
    if (x->kind != y->kind)
        return false;
    if (x->kind == ATOM_VAR) {
        for (unsigned i = 0; i < m->n; i++) {
            bool ea = var_id_eq(m->a[i], x->var_id), eb = var_id_eq(m->b[i], y->var_id);
            if (ea || eb)
                return ea && eb;
        }
        if (m->n == 512)
            return false;
        m->a[m->n] = x->var_id;
        m->b[m->n] = y->var_id;
        m->n++;
        return true;
    }
    if (x->kind == ATOM_EXPR) {
        if (x->expr.len != y->expr.len)
            return false;
        for (CettaExprLen i = 0; i < x->expr.len; i++)
            if (!s2_alpha(x->expr.elems[i], y->expr.elems[i], m))
                return false;
        return true;
    }
    return atom_eq((Atom *)x, (Atom *)y);
}

static Atom *s2_parse(const char *text) {
    size_t pos = 0;
    return parse_sexpr(&g_arena, text, &pos);
}

static bool s2_init(void) {
    if (g_init)
        return !g_init_failed;
    g_init = true;
    arena_init_detached(&g_arena);
    for (int f = 0; f < F_N; f++) {
        const S2FnDef *d = &g_defs[f];
        g_sym[f] = symbol_intern_cstr(g_symbols, d->name);
        for (unsigned i = 0; d->eqs[i]; i++) {
            if (i == 4 || !(g_eq_atoms[f][i] = s2_parse(d->eqs[i]))) {
                g_init_failed = true;
                return false;
            }
            g_eq_count[f] = i + 1;
        }
        g_decl_atom[f] = d->decl ? s2_parse(d->decl) : NULL;
        if (d->decl && !g_decl_atom[f]) {
            g_init_failed = true;
            return false;
        }
        char buf[256];
        int n = snprintf(buf, sizeof(buf), "(= (%s", d->name);
        for (unsigned i = 1; i <= d->arity; i++)
            n += snprintf(buf + n, sizeof(buf) - (size_t)n, " $cpk_s2_q%u", i);
        snprintf(buf + n, sizeof(buf) - (size_t)n, ") $cpk_s2_body)");
        g_eq_query[f] = s2_parse(buf);
        snprintf(buf, sizeof(buf), "(: %s $cpk_s2_type)", d->name);
        g_decl_query[f] = s2_parse(buf);
        if (!g_eq_query[f] || !g_decl_query[f]) {
            g_init_failed = true;
            return false;
        }
    }
    for (unsigned i = 0; i < S2_NCTORS; i++) {
        g_ctor_sym[i] = symbol_intern_cstr(g_symbols, g_ctors[i].name);
        g_ctor_decl[i] = g_ctors[i].decl ? s2_parse(g_ctors[i].decl) : NULL;
        char buf[160];
        snprintf(buf, sizeof(buf), "(: %s $cpk_s2_type)", g_ctors[i].name);
        g_ctor_decl_query[i] = s2_parse(buf);
        if ((g_ctors[i].decl && !g_ctor_decl[i]) || !g_ctor_decl_query[i]) {
            g_init_failed = true;
            return false;
        }
    }
    for (unsigned i = 0; g_builtins[i]; i++)
        g_builtin_sym[i] = symbol_intern_cstr(g_symbols, g_builtins[i]);
    s_and_bundle = symbol_intern_cstr(g_symbols, "and-bundle");
    s_cpu = symbol_intern_cstr(g_symbols, "CPU");
    s_stv = symbol_intern_cstr(g_symbols, "STV");
    s_ctv = symbol_intern_cstr(g_symbols, "CTV");
    s_pool_part = symbol_intern_cstr(g_symbols, "pool-part");
    s_marginal_proj = symbol_intern_cstr(g_symbols, "marginal-proj");
    s_proof_output_key = symbol_intern_cstr(g_symbols, "proof-output-key");
    s_quote = symbol_intern_cstr(g_symbols, "quote");
    s_stored_proof_row = symbol_intern_cstr(g_symbols, "stored-proof-row");
    s_cons = symbol_intern_cstr(g_symbols, "cons");
    s_proof_space = symbol_intern_cstr(g_symbols, "&proof");
    s_plus = symbol_intern_cstr(g_symbols, "+");
    s_times = symbol_intern_cstr(g_symbols, "*");
    s_lt = symbol_intern_cstr(g_symbols, "<");
    return true;
}

/* the rows of a one-pattern query, with the pattern instantiated per row */
static CettaCount s2_query(Space *sp, Arena *a, Atom *pattern, Atom **out, CettaCount cap) {
    Bindings seed;
    bindings_init(&seed);
    BindingSet rows;
    Atom *q = pattern;
    space_query_conjunction(sp, a, &q, 1, &seed, &rows);
    CettaCount n = rows.len;
    for (CettaCount i = 0; i < n && i < cap; i++)
        out[i] = bindings_apply(&rows.items[i], a, pattern);
    binding_set_free(&rows);
    bindings_free(&seed);
    return n;
}

/* the definition of f in &self is the compiled one */
static const char *s2_check_definition(Space *self, Arena *a, int f) {
    Atom *rows[8];
    CettaCount n = s2_query(self, a, g_eq_query[f], rows, 8);
    if (n != g_eq_count[f])
        return n < g_eq_count[f] ? "an equation of a compiled function is missing"
                                 : "an extra equation of a compiled function";
    for (CettaCount i = 0; i < n; i++) {
        S2VarMap m = {.n = 0};
        if (!rows[i] || !s2_alpha(rows[i], g_eq_atoms[f][i], &m))
            return "an equation of a compiled function differs from the compiled text";
    }
    n = s2_query(self, a, g_decl_query[f], rows, 8);
    if (n != (g_decl_atom[f] ? 1u : 0u))
        return "the type declaration of a compiled function changed";
    if (n) {
        S2VarMap m = {.n = 0};
        if (!rows[0] || !s2_alpha(rows[0], g_decl_atom[f], &m))
            return "the type declaration of a compiled function differs from the compiled text";
    }
    return NULL;
}

/* the type declaration of a constructor the code builds or dispatches on is the compiled one (or still absent) */
static const char *s2_check_ctor_decl(Space *self, Arena *a, unsigned i) {
    Atom *rows[4];
    CettaCount n = s2_query(self, a, g_ctor_decl_query[i], rows, 4);
    if (n != (g_ctor_decl[i] ? 1u : 0u))
        return "the type declaration of a constructor the code builds changed";
    if (n) {
        S2VarMap m = {.n = 0};
        if (!rows[0] || !s2_alpha(rows[0], g_ctor_decl[i], &m))
            return "the type declaration of a constructor the code builds differs from the compiled text";
    }
    return NULL;
}

/* no definition of any head the code relies on in a space other than &self */
static const char *s2_check_foreign_space(Space *sp) {
    if (space_equations_may_match_unnamed_head(sp))
        return "a catch-all equation in a space the code reads";
    for (int f = 0; f < F_N; f++)
        if (space_equations_may_match_known_head(sp, g_sym[f]))
            return "an equation of a compiled function in a space the code reads";
    for (unsigned i = 0; i < S2_NCTORS; i++)
        if (space_equations_may_match_known_head(sp, g_ctor_sym[i]))
            return "an equation of a constructor in a space the code reads";
    return NULL;
}

static void s2_add_space(Space *sp) {
    if (!sp)
        return;
    for (unsigned i = 0; i < g_val.nspaces; i++)
        if (g_val.spaces[i] == sp)
            return;
    if (g_val.nspaces < S2_MAX_SPACES) {
        g_val.spaces[g_val.nspaces] = sp;
        g_val.toks[g_val.nspaces] = space_program_token(sp);
        g_val.nspaces++;
    }
}

static void s2_revalidate(const CettaS2FnCall *call) {
    g_revalidations++;
    Space *self = call->self;
    memset(&g_val, 0, sizeof(g_val));
    g_val.have = true;
    g_val.self_tok = space_program_token(self);
    /* the data spaces the code reads (imported modules are part of &self: their atoms are &self's imported prefix
     * or its overlay base, both covered by &self's program token) */
    Atom *ref = atom_symbol_id(call->arena, s_proof_space);
    Space *proof = ref ? call->resolve(call->ctx, ref) : NULL;
    if (proof && proof != self)
        s2_add_space(proof);
    /* &self's imported modules (HE-style module dependencies): queries on &self read their equations, so their
     * revisions are tracked as well (a mutation in a module also advances &self's revisions; this does not rely on
     * that) */
    uint32_t ndeps = space_dependency_count(self);
    g_val.ndeps = ndeps;
    for (uint32_t d = 0; d < ndeps; d++)
        s2_add_space(space_dependency_at(self, d));
    if (g_trace)
        fprintf(stderr, "s2-fn revalidate #%" PRIu64 ": &self deps %u imported_len %llu, spaces tracked %u%s\n",
                g_revalidations, (unsigned)ndeps, (unsigned long long)space_imported_length(self), g_val.nspaces,
                g_val.nspaces >= S2_MAX_SPACES ? " (TRUNCATED)" : "");
    const char *global = NULL;
    if (ndeps + (proof ? 1u : 0u) > S2_MAX_SPACES)
        global = "more spaces to track than the guard holds";
    if (space_equations_may_match_unnamed_head(self))
        global = "a catch-all equation in &self";
    for (unsigned i = 0; !global && g_builtins[i]; i++)
        if (space_equations_may_match_known_head(self, g_builtin_sym[i]))
            global = "an equation for a builtin the compiled code reproduces";
    for (unsigned i = 0; !global && i < S2_NCTORS; i++)
        if (space_equations_may_match_known_head(self, g_ctor_sym[i]))
            global = "an equation for a constructor the compiled code builds";
    for (unsigned i = 0; !global && i < g_val.nspaces; i++)
        global = s2_check_foreign_space(g_val.spaces[i]);
    Arena a;
    arena_init_detached(&a);
    for (unsigned i = 0; !global && i < S2_NCTORS; i++)
        global = s2_check_ctor_decl(self, &a, i);
    const char *own[F_N];
    for (int f = 0; f < F_N; f++)
        own[f] = global ? global : s2_check_definition(self, &a, f);
    arena_free(&a);
    for (int f = 0; f < F_N; f++) {
        const char *why = own[f];
        for (unsigned k = 0; !why && k < 12 && g_defs[f].deps[k] >= 0; k++)
            why = own[g_defs[f].deps[k]];
        g_val.valid[f] = why == NULL;
        g_val.reason[f] = why;
    }
}

static bool s2_current(const CettaS2FnCall *call) {
    if (!g_val.have || g_val.self_tok.space != call->self ||
        !space_program_token_eq(g_val.self_tok, space_program_token(call->self)))
        return false;
    for (unsigned i = 0; i < g_val.nspaces; i++)
        if (!space_program_token_eq(g_val.toks[i], space_program_token(g_val.spaces[i])))
            return false;
    if (g_val.ndeps != space_dependency_count(call->self))
        return false;
    /* &proof bound after the last validation */
    if (g_val.nspaces == 0) {
        Atom *ref = atom_symbol_id(call->arena, s_proof_space);
        if (ref && call->resolve(call->ctx, ref))
            return false;
    }
    return true;
}

/* ---- entry --------------------------------------------------------------------------------------------- */

static void s2_stats(void);

bool cetta_s2_fns_active(void) {
    if (g_mode < 0) {
        const char *e = getenv("CETTA_S2_FNS");
        g_mode = !e || !*e || !strcmp(e, "0") ? 0 : !strcmp(e, "invalid") ? 2 : 1;
        const char *t = getenv("CETTA_S2_FNS_TRACE");
        g_trace = t && *t ? atoi(t) : 0;
        if (g_mode)
            atexit(s2_stats);
    }
    return g_mode != 0;
}

bool cetta_s2_fns_claims(SymbolId head) {
    if (!s2_init())
        return false;
    for (int f = 0; f < F_N; f++)
        if (g_sym[f] == head && g_defs[f].entry)
            return true;
    g_offers_unclaimed++;
    return false;
}

int cetta_s2_fns_try(const CettaS2FnCall *call, Atom *call_atom, Atom **answer) {
    if (!cetta_s2_fns_active() || !s2_init() || !call_atom || call_atom->kind != ATOM_EXPR ||
        call_atom->expr.len == 0)
        return CETTA_S2_FN_DECLINED;
    int f = -1;
    for (int i = 0; i < F_N; i++)
        if (g_defs[i].entry && s2_is_sym(call_atom->expr.elems[0], g_sym[i])) {
            f = i;
            break;
        }
    if (f < 0 || call_atom->expr.len != g_defs[f].arity + 1)
        return CETTA_S2_FN_DECLINED;
    g_st[f].offered++;
    if (g_mode == 2) {
        g_st[f].declined_forced++;
        return CETTA_S2_FN_DECLINED;
    }
    if (!s2_current(call))
        s2_revalidate(call);
    if (!g_val.valid[f]) {
        g_st[f].declined_invalid++;
        s2_note(g_val.reason[f]);
        if (g_trace)
            fprintf(stderr, "s2-fn %s: invalid: %s\n", g_defs[f].name, g_val.reason[f]);
        return CETTA_S2_FN_DECLINED;
    }
    S2Ctx c = {.call = call, .a = call->arena, .decline = NULL, .depth = 0};
    Atom *x[2] = {NULL, NULL};
    for (unsigned i = 0; i < g_defs[f].arity; i++)
        x[i] = s2_observe(call->arena, call_atom->expr.elems[1 + i]);
    Atom *r = NULL;
    switch (f) {
    case F_MAKE_AND_BUNDLE:
        /* the operands as the call holds them (CeTTa's answer holds them as they are) */
        r = fn_make_and_bundle(&c, call_atom->expr.elems[1], call_atom->expr.elems[2]);
        break;
    case F_AND_BUNDLE_PREMISES:
        r = fn_and_bundle_premises(&c, x[0]);
        break;
    case F_FRONTIER_REMAINING_SCORE:
        r = fn_frontier_remaining_score(&c, x[0], x[1]);
        break;
    case F_FRONTIER_POOL_POTENTIAL:
        r = fn_frontier_pool_potential(&c, x[0]);
        break;
    case F_EXPR_ARITY:
        r = fn_expr_arity(&c, x[0]);
        break;
    case F_PREMISES_EXPECTED_CONFIDENCE:
        r = fn_premises_expected_confidence(&c, x[0]);
        break;
    case F_PREMISES_EXPECTED_CONFIDENCE_STEP:
        r = fn_premises_expected_confidence_step(&c, x[0], x[1]);
        break;
    case F_TERM_CONFIDENCE:
        r = fn_term_confidence(&c, x[0]);
        break;
    case F_PROOF_SCORE:
        r = fn_proof_score(&c, x[0], x[1]);
        break;
    case F_TV_CONFIDENCE:
        r = fn_tv_confidence(&c, x[0]);
        break;
    case F_PROOF_OUTPUT_KEY_BY_ID:
        r = fn_proof_output_key_by_id(&c, x[0]);
        break;
    case F_PROOF_OUTPUT_ATOM_BY_ID:
        r = fn_proof_output_atom_by_id(&c, x[0]);
        break;
    case F_CANDIDATE_OUTPUT_KEY:
        r = fn_candidate_output_key(&c, x[0]);
        break;
    default:
        break;
    }
    if (!r) {
        g_st[f].declined_call++;
        s2_note(c.decline ? c.decline : "unknown");
        if (g_trace) {
            fprintf(stderr, "s2-fn %s: declined: %s: ", g_defs[f].name, c.decline ? c.decline : "?");
            if (g_trace > 1)
                atom_print(call_atom, stderr);
            fputc('\n', stderr);
        }
        return CETTA_S2_FN_DECLINED;
    }
    g_st[f].answered++;
    if (g_trace > 2) {
        fprintf(stderr, "s2-fn %s: answered ", g_defs[f].name);
        atom_print(r, stderr);
        fputc('\n', stderr);
    }
    *answer = r;
    return CETTA_S2_FN_ANSWER;
}

static void s2_stats(void) {
    const char *path = getenv("CETTA_S2_FNS_STATS");
    if (!path || !*path)
        return;
    FILE *out = fopen(path, "w");
    if (!out)
        return;
    fprintf(out, "{\"schema\": \"cpk-s2-fns-v1\", \"mode\": %d, \"revalidations\": %" PRIu64 ", \"functions\": {",
            g_mode, g_revalidations);
    for (int f = 0; f < F_N; f++)
        fprintf(out,
                "%s\"%s\": {\"offered\": %" PRIu64 ", \"answered\": %" PRIu64 ", \"declined_invalid\": %" PRIu64
                ", \"declined_forced\": %" PRIu64 ", \"declined_call\": %" PRIu64 "}",
                f ? ", " : "", g_defs[f].name, g_st[f].offered, g_st[f].answered, g_st[f].declined_invalid,
                g_st[f].declined_forced, g_st[f].declined_call);
    fprintf(out, "}, \"reasons\": {");
    for (unsigned i = 0; i < S2_NREASONS && g_reasons[i].why; i++)
        fprintf(out, "%s\"%s\": %" PRIu64, i ? ", " : "", g_reasons[i].why, g_reasons[i].n);
    fprintf(out, "}}\n");
    fclose(out);
}
#endif
