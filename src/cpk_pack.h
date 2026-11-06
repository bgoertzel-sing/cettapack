/* CettaPack M1: pack loading (manifest, digests), the guard, the hosted
 * callable and the residual fallback. */
#ifndef CETTAPACK_PACK_H
#define CETTAPACK_PACK_H

#include "cetta_embed.h"
#include "cpk.h"
#include "cpk_support.h"

typedef struct {
    const CpkPlan *plan;
    const char *manifest_path;
    const char *site_path;
    const char *language;          /* engine language, part of the live profile */
    bool pathmap_enabled;          /* test item 14 turns this off */
    CpkDemand demand;
} CpkPackOptions;

typedef struct {
    CpkPackOptions opt;
    cetta_engine_t *engine;
    CpkJson *manifest;
    bool loaded;                   /* fail closed: false when any hash is missing */
    char load_error[256];

    /* live digests, computed at load */
    char live_profile[65], live_langdef[65], live_region[65], live_pattern[65],
         live_template[65], live_schema[65];
    /* manifest digests (borrowed from the parsed manifest) */
    const char *m_profile, *m_langdef, *m_region, *m_pattern, *m_template, *m_schema;
    bool digests_match;
    char digest_mismatch[160];

    /* the compiled region, parsed by CeTTa from the site source */
    cetta_atom_t *region_handle;
    bool region_ok;                /* the residual needs only the region */
    Atom *slot_vars[CPK_MAX_SLOTS];
    Atom *factor_atoms[CPK_MAX_FACTORS];
    Atom *conj_atom, *template_atom;

    CpkIndex *index;               /* narrowing index for the instantiated space */
    uint64_t instantiate_ns;
    CpkStats stats;
    CpkVerdict last_verdict;
    char last_reason[200];
    cetta_atom_t *callable;
} CpkPack;

/* Load a pack and register its callable (named after the site). The callable
 * is registered even when loading fails, so the site still answers through
 * the residual; a pack that did not load never runs its kernel. */
cetta_status_t cpk_pack_open(cetta_engine_t *engine, const CpkPackOptions *opt, CpkPack **out);
void cpk_pack_close(CpkPack *pack);

/* Canonical structural text and its digest (cpk-struct-v1). */
bool cpk_struct_digest(Atom *atom, char out[65], char **canon_out);

/* Live profile digest for a language (cpk-profile-v1). */
void cpk_live_profile_digest(const char *language, char out[65], char *components, size_t len);

/* Schema digest of a plan (cpk-schema-v1). */
void cpk_plan_schema_digest(const CpkPlan *plan, char out[65]);

const char *cpk_verdict_name(CpkVerdict v);
#endif
