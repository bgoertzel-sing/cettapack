# CettaPack M1: design, and how it maps to the note

Source: Ben Goertzel, *CeTTa-Hosted Guarded Supercompilation for MeTTa*
(2026-07-06), `docs/cetta-guarded-supercompilation-2026-07-06.pdf` (sha256 in
docs/PINS.md). Section numbers below are the note's. A first version of M1 was
built before the note was readable; this version is aligned with it, and the
table says where M1 follows the note and where it adapts.

## Conformance with appendix A

| Note | M1 | Where |
|---|---|---|
| A.1 prime directive: guard and fallback for every kernel; refuse, don't guess | the site is a CeTTa callable; every call is guarded; any non-VALID verdict or kernel error runs the residual; unsupported regions are refused by the compiler | `src/cpk_pack.c` (`compute`), `tools/cpkc.py` |
| A.2 manifest v0.2 | same layout and field names (`artifact_kind`, `manifest_version`, `identity`, `effects`, `spaces`, `guards`, `artifacts`, `amortization`); M1-specific data under `x_cettapack`. Values M1 does not compute say so in place (`stats_signature`, `native_so_hash` = null: the kernel is linked into the host, its plan is hashed as `native_plan_hash`) | `packs/target_expressed/manifest.json` |
| A.3 guard | same order: profile, langdef, meta-expanded region hash, match pattern hash, then per space schema hash and revision policy; plus template and normalized-region hashes, input and backend checks. Fail closed: a missing or malformed identity hash, schema hash or space entry means the pack does not load | `guard` in `src/cpk_pack.c` |
| A.3 revision policy `append_only_epoch` | the index keeps the space's record ids at instantiation. Same revision: VALID path. Records appended (old records unchanged, in order) within `max_append_delta_atoms`: **PATCHABLE**; over budget: **REPLAN**; anything else (removal, rewrite): **INVALID** | `cpk_index_change` in `src/cpk_kernel.c` |
| A.3 stats drift -> REPLAN | not computed in M1 (stub, never raised) | |
| A.4 patch lifecycle | PATCHABLE: `apply_patches` is a stub that applies nothing, so the re-check cannot reach VALID; the call falls back (replan is not scheduled: M2) | `compute` |
| A.5 structural hash | parse by CeTTa (`cetta_embed_parse`); imports: none visible to the span; macro and meta-rule expansion: identity (the region holds none); alpha-normalize: variables renamed `$_0, $_1, ...` by first occurrence; serialize canonically; hash with one domain tag per field (`cpk-normalized-v1`, `cpk-meta-expanded-v1`, `cpk-pattern-v1`, `cpk-template-v1`). Operator schema IDs: `operator_schema_hashes`, one per relation. The source span hash is recorded but not guarded (it changes with comments: not semantic) | `tools/cpkc.py`, `pack_load` |
| A.6 kernel interface | `cpk_kernel_run(const CpkKernelCall *)` with a visitor `CpkVisitCtl (*)(void *user_data, Atom *answer, uint64_t multiplicity)` returning CONTINUE / STOP / ERROR. Borrows space, index, arena and inputs; owns no global state; never writes the space | `src/cpk.h` |
| A.7 algorithm | PathMap prefix iteration, then native seeded rematch on every candidate, template instantiation, multiplicity read from the records; for the conjunction, multiplicity is composed after rematch confirmed compatibility | `join` in `src/cpk_kernel.c` |
| A.8 certificate | `supercompile-cert` s-expression with every listed field and the eight steps, each with the evidence behind it; claim level `bag-tested-adequate`; open obligations listed; no proof | `packs/target_expressed/certificate.metta` |
| A.9 tests | items 1-7, 9-16 run and pass; item 8 (cross-space join) is not supported in M1 and the compiler refuses it (`tests/cross_space_site.metta`) | `make test` |
| A.10 anti-patterns | PathMap keys are never read as bindings (only a group id is read); no dedup unless demand = support; structural hashes always checked; profile includes the CeTTa pin and patch series; guard runs on every call; benchmark includes guard and fallback; residual kept | |
| A.11 definition of done | see the result report | |

## The site

`packs/target_expressed/site.metta`:

```metta
(= (cpk-site-target-expressed $space $rs)
   (match $space (, (target $rs $gene) (expressed $gene $tissue)) ($gene $tissue)))
```

A conjunction of two patterns over one space, a seeded input, a join variable
and a result template. `tools/cpkc.py` accepts equations of this shape with
flat symbol/variable factors, picks the join order (most ground fields first:
§10 step 2, "most selective grounded prefix") and emits a static plan
(`kernel_plan.c`) for the kernel runtime plus the manifest.

## A call (§10 steps 1-9)

1. **Instantiation, once per space.** Records of the site's relations are
   grouped by identical atom; each distinct atom is stored once in a
   PathMap-backed MORK space (CeTTa's bridge) under the key
   `(<atom> <group-id>)`, and a table maps each group to its source record
   positions. The space's CeTTa instance id (`space_instance_id`, so a freed
   space whose address is reused is not mistaken for the original), record
   ids, revision and length are kept for the dependency vector and the
   revision policy. A schema check runs over every record and yields the
   live-space schema digest (below).
2. **Guard** (A.3, above), timed on every call.
3. **Narrowing.** Per factor, the kernel writes the key prefix (constants,
   seeded inputs and variables bound by earlier factors, up to the first
   unbound field) and descends the PathMap trie with the bridge cursor; only
   the group id at the end of each key is read. Keys whose symbols MORK would
   intern (64 bytes or longer) are narrowed with MORK's own query instead.
4. **Native seeded rematch** on every candidate:
   `space_match_backend_match_atom_seeded` (`src/space_match_backend.c:12406`
   at the CeTTa pin), read from the candidate's first source record. All
   records of a group are the same ground all-symbol atom (schema check), so
   the rematch result is the same for each.
5. **Multiplicity (§10.1).** After a successful rematch the candidate's
   multiplicity is its number of source records; across the join the
   multiplicities multiply (product composition after exact compatibility).
   The visitor receives each answer once with that multiplicity. Under demand
   `support` it receives each distinct answer once with multiplicity 1.
6. **Answers.** The pack's visitor turns multiplicity m into m occurrences in
   CeTTa's result bag (that is how a CeTTa bag carries multiplicity) and copies
   them once into an engine-owned handle (`cetta_embed_native_atom_copy`).
7. **Fallback** (step 9): generic CeTTa evaluating
   `(match <space> <conjunction, inputs substituted> <template>)` built from
   the parsed region.

The schema check fails (INVALID) when a record could unify with a factor but
lies outside the compiled schema: a variable record, an expression with a
non-symbol head and a factor's arity, a record under a factor's head with
another arity, or one with a non-symbol field. That is what makes "candidates
come from a symbol trie" sound, and what lets one rematch stand for all
records of a group.

**Two schema checks (review 1, finding 4).** The manifest's
`spaces[0].schema_hash` is the compiled schema: `cpk-schema-v1` over the site's
relations (`head/arity:sym,...`). The guard compares it twice:

1. with the digest recomputed from the loaded plan (catches a manifest built
   for another plan; only a wrong manifest can fail it, item 10);
2. with the **live-space schema digest**, computed at instantiation from the
   records themselves: the same `cpk-schema-v1` text, with one
   `off-schema:<signature>` line appended per distinct off-schema signature
   seen (variable record, variable head, wrong arity under a factor head,
   non-symbol field). A conforming space hashes to the compiled value; any
   violation changes the digest and the guard returns INVALID (items 10r,
   10b, 10c). A relation with no records is vacuously conforming. After an
   append the revision policy routes to PATCHABLE or REPLAN before this check,
   and both fall back in M1, so appended records are never served unchecked.

## Correctness contract (§10.2), as M1 meets it

| Contract | M1 |
|---|---|
| PathMap candidate selector is sound | claimed for the guarded fragment (schema check), tested (items 1-7, 15, 16); not proved |
| native rematch is exact on the accepted fragment | CeTTa's own function; not re-verified |
| template instantiation matches generic CeTTa | `bindings_apply` on the parsed template; tested (item 7) |
| multiplicity reconstructed from space records | record counts and products; tested (items 4, 15) |
| fallback is identical to generic evaluation | the residual is generic `match`; tested on every guard failure item |

## Known limits

- One space per site; cross-space joins are refused (item 8).
- Patching and replanning are stubs: after any change to the space, that pack
  instance answers through the residual.
- Stats drift is not measured.
- The narrowing index costs memory: 564 MiB peak on the 128k KB, against
  42 MiB for generic CeTTa.
- Speed: the kernel is not faster than generic CeTTa on this site
  (docs/BENCHMARK.md).
