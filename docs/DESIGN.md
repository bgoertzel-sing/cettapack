# CettaPack M1: design and its adaptations

**Status of the source note.** The order's design note
(`cetta-guarded-supercompilation-2026-07-06.pdf`, appendix A) lives in the
hive's workspace repository, which this host cannot read; I asked for it on the
record (blocked, needs information) and built M1 from the order's own
restatement of A.11. Every shape below that the note defines (A.2 manifest
v0.2, A.3 guard, A.5 structural hashing, A.6 kernel interface, A.8 certificate,
A.9 test list, §10.1 multiplicity, §17 benchmark) is therefore **my
adaptation**, named as such. When the note is available these should be
checked against it; each is small and isolated in one file.

## The site

`packs/target_expressed/site.metta`:

```metta
(= (cpk-site-target-expressed $space $rs)
   (match $space (, (target $rs $gene) (expressed $gene $tissue)) ($gene $tissue)))
```

A conjunction of two patterns over one space, with a seeded input (`$rs`), a
join variable (`$gene`) and a result template. The compiler
(`tools/cpkc.py`) accepts any equation of this shape whose factors are flat
symbol/variable patterns; it chooses the join order (most ground fields first)
and emits a static plan (`packs/target_expressed/kernel_plan.c`) for the kernel
runtime (`src/cpk_kernel.c`). "Meta-expansion" of the region is the identity in
M1: the region contains no macros.

## How a call runs

`cpk-site-target-expressed` is registered as a CeTTa embedding callback, so
`(cpk-site-target-expressed &kb rs5)` is evaluated by CeTTa like any other
function. On each call (`src/cpk_pack.c`, `compute`):

1. **Instantiation (once per space).** The first call with a space builds the
   narrowing index: every record of the space is classified against the
   compiled schema; records of the site's relations are grouped by identical
   atom text, and each distinct atom is stored once in a PathMap-backed MORK
   space (CeTTa's bridge, `cetta_mork_bridge_space_new`) under the key
   `(<atom> <group-id>)`. A CSR table maps each group id to its source record
   positions. The index records the space, its revision and its length.
   Index build time is reported separately from per-call time.
2. **Guard (every call, timed).** See below. Not VALID means fallback.
3. **Kernel.** For each factor in plan order, the kernel writes the key prefix
   (wrapper, arity, then the ground fields: constants, seeded inputs and
   variables bound by earlier factors, up to the first unbound field) and
   descends the PathMap trie byte by byte with CeTTa's bridge cursor
   (`cetta_mork_bridge_cursor_descend_byte`, `..._next_val`, `..._path_bytes`).
   The only thing read from PathMap is the group id at the end of each key.
   Then, for **every source record** of every candidate group, it calls CeTTa's
   native seeded rematch
   `space_match_backend_match_atom_seeded` (`src/space_match_backend.c:12406` at
   the pin, declared `src/space_match_backend.h:569`; it calls
   `match_space_atom_epoch`, `:1004`, which calls `match_atoms_atom_id_epoch`,
   `src/match.c:12440`) with a clone of the current bindings. Bindings come only
   from rematch. When all factors matched, the template is instantiated with
   `bindings_apply` and emitted.
4. **Answer construction.** Answers are deep-copied once into an engine-owned
   handle (`cetta_embed_native_atom_copy`, the upstream patch) and returned one
   occurrence per callback pull.
5. **Fallback.** Any guard failure, and any kernel error, runs the residual:
   generic CeTTa evaluating `(match <space> <conjunction with inputs
   substituted> <template>)` built from the parsed source region.

If the narrowing key prefix cannot be written byte for byte (a symbol that is
not plain text or is 64 bytes or longer, which MORK would intern), the kernel
uses MORK's own query (`cetta_mork_bridge_space_query_bindings_text`) for that
factor and still reads only the group id from it. On this KB the byte path is
always taken (the index self-check confirms every key decodes).

### Multiplicity (§10.1, adapted)

Duplicates are separate records of the source space. The index groups records
by atom only to keep the trie small; the kernel then rematches and emits per
record, so k copies of a fact give k answers, and a join of a k-fold and an
m-fold fact gives k·m answers, exactly as generic CeTTa's `match` does. The
kernel deduplicates only under demand mode `support` (test 4s).

### Ownership

The kernel (`cpk_kernel_run`) receives everything through `CpkKernelCall`:
the space, the index, a scratch arena, the bindings' variables, the inputs, the
sink and the statistics belong to the caller. It has no static mutable state;
its PathMap cursor lives for one call. It never writes the space: tests check
the space's length before and after every kernel call, and the guard would see
any revision change.

## Kernel interface (A.6, adapted)

`src/cpk.h`:

```c
int cpk_kernel_run(const CpkKernelCall *call);   /* 0 = complete; nonzero = caller falls back */

typedef struct {
    const CpkPlan *plan;          /* compiled site plan */
    Space *space;                 /* borrowed, read-only */
    const CpkIndex *index;        /* borrowed narrowing index */
    Arena *scratch;               /* borrowed */
    Atom *const *slot_vars;       /* the region's variables */
    Atom *const *factor_atoms;    /* the region's factor patterns (rematched as is) */
    Atom *template_atom;
    Atom *const *inputs;          /* callable arguments */
    CpkDemand demand;             /* BAG or SUPPORT */
    CpkEmitFn emit; void *sink;   /* one call per answer occurrence */
    CpkStats *stats;              /* guard/narrow/rematch/answer timings and counts */
} CpkKernelCall;
```

## Manifest (A.2 v0.2 shape, adapted)

`packs/target_expressed/manifest.json`, written by `tools/cpkc.py`. Fields the
loader requires (missing or malformed: **the pack does not load**, and the site
then always runs the residual): `cettapack: "0.2"`, `site.name`,
`profile.digest`, `langdef.digest`, `structure.region_hash`,
`structure.pattern_hash`, `structure.template_hash`, `schema.digest`, and a
non-empty `dependencies.spaces` vector.

| Digest | Method | Live value computed from |
|---|---|---|
| profile | `cpk-profile-v1`: sha256 of CeTTa commit, patch-series digest, BUILD mode, engine language | compile-time constants of the host + the engine's language |
| langdef | `cpk-tree-v1`: sha256 over `langdef/<tree>` files (path, sha256 of content), sorted | the CeTTa tree the host was built from |
| structure (region, pattern, template) | `cpk-struct-v1` (A.5 adaptation): sha256 of canonical text, variables renamed `$_0, $_1, ...` by first occurrence in the hashed unit, one space between children | the site source as **parsed by CeTTa** (`cetta_embed_parse`), canonicalized through the embedding API |
| schema | `cpk-schema-v1`: each factor relation as `head/arity:sym,...` | the compiled plan |

Python (compiler) and C (loader) compute the structural hashes independently;
`build/cettapack-host hashes` prints both sides and `make test` runs it.

## Guard (A.3, adapted)

Checked on every call, in this order (`src/cpk_pack.c`, `guard`):

| Check | Failure verdict |
|---|---|
| pack loaded (fail closed) | INVALID |
| profile, langdef, structural, schema digests equal the manifest's | INVALID |
| the space argument is a space; seeded inputs are ground symbols | INVALID |
| the space's backend takes seeded candidates (`space_match_backend_supports_seeded_candidates`) | INVALID |
| index exists and was built for this space (space dependency vector) | INVALID |
| space revision and length equal the indexed ones | **PATCHABLE (M1 stub: falls back)** |
| schema check of the space passed at instantiation | INVALID |
| PathMap narrowing enabled and its index available | INVALID |
| otherwise | VALID |

PATCHABLE and REPLAN are stubs: both fall back exactly like INVALID. A changed
space is not re-indexed in M1 (that is loader lifecycle, M2): after any change,
that pack instance answers through the residual.

The schema check at instantiation fails when any record could unify with a
factor but lies outside the compiled schema: a variable record, an expression
with a non-symbol head and a factor's arity, a record under a factor's head
with another arity, or one with a non-symbol field (number, variable,
expression). That is what makes "candidates come from a symbol trie" safe.

## Certificate skeleton (A.8, adapted)

`make test` writes `packs/target_expressed/certificate.json` only if every
test item passed: pack id, manifest and plan digests, structural hashes,
claim level `bag-tested-adequate`, the guard conditions under which the claim
is made, the evidence (test items, log digest, KB digest and seed, commits),
`proof: null` and the open obligations. No proof is claimed; Lean is not part
of M1.

## Differential tests (A.9)

`build/cettapack-host test --kb build/kb.metta` (`make test`). Every item runs
generic CeTTa's `match` and the site callable in the same engine on the same
space and compares the two answer bags as sorted multisets. It also checks the
guard verdict, which path ran (kernel or fallback), the answer count and that
the space did not change.

The item numbers follow the order's list (the note's numbering, which I could
not read): 1 ground match, 2 no result, 3 multiple results, 4 duplicates and
multiplicity, 5 co-reference, 6 distinct variables, 7 result template, 9
profile mismatch, 10 schema mismatch, 11 structural-hash mismatch, 14 PathMap
disabled, 15 comparison with generic CeTTa (300 seeds over the 128,576-atom
KB). Extra items: 2b, 4s (support demand), 10b, 10c, 11b (missing hash: fail
closed), `dep` (space changed after instantiation), `in` (non-symbol input).

**Not in M1:** items 8, 12, 13 and 16. I could not read their definitions in
the note, so I did not guess at them.

## Known limits

- The narrowing index is built per pack instance and is not updated; any
  change to the space sends that instance to the residual (PATCHABLE stub).
- The index costs memory: on the 128k KB the kernel process peaks at
  563 MiB against 42 MiB for generic CeTTa, almost all of it the MORK space
  (jemalloc arenas included). See docs/BENCHMARK.md.
- It is not faster than generic CeTTa on this site (0.83x end to end, guards
  included); docs/BENCHMARK.md says where the time goes.
- The residual is built from the region CeTTa parsed. If the site source
  cannot be parsed in the engine's language, the callable cannot answer at
  all (it fails rather than guess).
- One site is compiled. The compiler covers flat conjunctions over one space;
  two spaces, nested patterns and non-symbol fields are refused at compile
  time or by the schema guard.
