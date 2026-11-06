/* CettaPack M1: guarded, site-specific match kernels hosted in CeTTa.
 *
 * Kernel interface. This is the documented adaptation of the design note's
 * A.6 interface (see docs/DESIGN.md, "Kernel interface"):
 *
 *   - the kernel borrows everything: the space, the narrowing index, a scratch
 *     arena, the seeded inputs and the answer sink belong to the caller;
 *   - it owns no global state and never mutates a space;
 *   - PathMap/MORK is used only to select candidate source records; every
 *     answer is decided by CeTTa's native seeded rematch
 *     (space_match_backend_match_atom_seeded) on each candidate record;
 *   - multiplicity comes from the source records: each record of a candidate
 *     atom is rematched and emitted on its own, so k copies give k answers;
 *     answers are deduplicated only under CPK_DEMAND_SUPPORT.
 */
#ifndef CETTAPACK_CPK_H
#define CETTAPACK_CPK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "atom.h"
#include "space.h"

#define CPK_MAX_FACTORS 4u
#define CPK_MAX_FIELDS 8u
#define CPK_MAX_SLOTS 16u

typedef enum {
    CPK_VALID = 0,      /* use the kernel */
    CPK_INVALID = 1,    /* fall back to the residual (generic CeTTa) */
    CPK_PATCHABLE = 2,  /* M1 stub: treated as INVALID (fallback) */
    CPK_REPLAN = 3      /* M1 stub: treated as INVALID (fallback) */
} CpkVerdict;

typedef enum {
    CPK_DEMAND_BAG = 0,     /* every occurrence, multiplicities kept */
    CPK_DEMAND_SUPPORT = 1  /* distinct answers only */
} CpkDemand;

/* ---- site plan (emitted by tools/cpkc.py into packs/<site>/kernel.c) ---- */

typedef enum { CPK_FIELD_CONST = 0, CPK_FIELD_SLOT = 1 } CpkFieldKind;

typedef struct {
    uint8_t kind;          /* CpkFieldKind */
    uint8_t slot;          /* variable slot, for CPK_FIELD_SLOT */
    const char *text;      /* symbol text, for CPK_FIELD_CONST */
} CpkField;

typedef struct {
    uint8_t conj_index;            /* position of this factor in the source conjunction */
    uint8_t arity;                 /* fields, head included */
    CpkField fields[CPK_MAX_FIELDS];
} CpkFactor;

typedef struct {
    const char *site;              /* site name, also the callable's name */
    uint8_t nslots;                /* variables of the region, first-occurrence order */
    const char *slot_names[CPK_MAX_SLOTS];
    uint8_t nfactors;
    CpkFactor factors[CPK_MAX_FACTORS];   /* join order chosen by the compiler */
    uint8_t space_input;           /* input position of the space argument */
    uint8_t ninputs;               /* callable arity */
    int8_t input_slot[CPK_MAX_SLOTS];     /* input position -> slot, -1 for the space */
    const char *region_text;       /* the source region the kernel was compiled from */
} CpkPlan;

/* ---- narrowing index (PathMap/MORK), owned by a pack instance ---- */

typedef struct CpkIndex CpkIndex;

/* ---- statistics, owned by the caller ---- */

typedef struct {
    uint64_t calls, kernel_calls, fallbacks;
    uint64_t guard_ns, narrow_ns, rematch_ns, answer_ns, fallback_ns, kernel_ns;
    uint64_t narrow_queries;
    uint64_t candidates;        /* distinct candidate atoms returned by narrowing */
    uint64_t rematches;         /* source records rematched natively */
    uint64_t rematch_ok;        /* rematches that succeeded */
    uint64_t answers;           /* answers emitted (occurrences) */
} CpkStats;

/* The sink receives each answer occurrence; the atom is borrowed. A nonzero
 * return aborts the kernel, and the caller then falls back. */
typedef int (*CpkEmitFn)(void *sink, Atom *answer);

typedef struct {
    const CpkPlan *plan;
    Space *space;               /* borrowed, read-only */
    const CpkIndex *index;      /* borrowed */
    Arena *scratch;             /* borrowed */
    Atom *const *slot_vars;     /* borrowed: the region's variables by slot */
    Atom *const *factor_atoms;  /* borrowed: the region's factor patterns */
    Atom *template_atom;        /* borrowed: the result template */
    Atom *const *inputs;        /* borrowed: callable arguments */
    CpkDemand demand;
    CpkEmitFn emit;
    void *sink;
    CpkStats *stats;
} CpkKernelCall;

/* Returns 0 when the kernel ran to completion; nonzero when it could not
 * (out of memory, bridge error, sink abort). The caller must then discard
 * any emitted answers and fall back. */
int cpk_kernel_run(const CpkKernelCall *call);

/* Index lifecycle. Build reads the space's records once, never writes. */
CpkIndex *cpk_index_build(const CpkPlan *plan, Space *space, Arena *scratch,
                          bool pathmap_enabled, char *why, size_t why_len);
void cpk_index_free(CpkIndex *index);
bool cpk_index_schema_ok(const CpkIndex *index);
bool cpk_index_pathmap_ok(const CpkIndex *index);
Space *cpk_index_space(const CpkIndex *index);
uint64_t cpk_index_revision(const CpkIndex *index);
uint64_t cpk_index_length(const CpkIndex *index);
uint64_t cpk_index_groups(const CpkIndex *index);
uint64_t cpk_index_rows(const CpkIndex *index);
const char *cpk_index_schema_note(const CpkIndex *index);

uint64_t cpk_now_ns(void);

#endif
