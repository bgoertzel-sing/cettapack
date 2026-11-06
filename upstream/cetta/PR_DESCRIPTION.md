# PR: embedding: in-process native borrowing for compiled kernels

Patch: `0001-embedding-in-process-native-borrowing-for-compiled-k.patch`
Applies to: zariuq/CeTTa `4c285a97fd8c60e3c3fccd5ccd3b3e5e0ebccb4c` (`git am`).
Size: 3 files, +69/-1 (new header, three functions in `src/embedding.c`, one
Makefile dependency).

## What it adds

`include/cetta_embed_native.h`:

```c
const struct Atom *cetta_embed_native_atom(const cetta_atom_t *atom);
struct Space *cetta_embed_native_space(const cetta_atom_t *atom);
cetta_status_t cetta_embed_native_atom_copy(cetta_engine_t *engine,
                                            const struct Atom *value,
                                            cetta_atom_t **output);
```

## Why it is needed

A CettaPack kernel runs inside a registered embedding callback, so it is
evaluated by CeTTa like any function and it borrows the caller's state. To
narrow candidates and then call CeTTa's own seeded rematch
(`space_match_backend_match_atom_seeded`) it needs the `Space *` and the
`Atom *` behind the callback's argument handles, and to return its answers it
needs to wrap runtime atoms into engine-owned handles. The stable API exposes
neither: `struct cetta_atom`, `struct cetta_space`, `atom_wrap` and
`owner_new` are private to `src/embedding.c`. Without the patch a host would
have to copy those private struct layouts, which breaks silently when they
change.

## What it does not do

- It changes no existing function and no stable-ABI symbol's behaviour.
- Nothing in it mutates a space; `cetta_embed_native_space` hands out a
  pointer the caller must treat as read-only.
- It is only meaningful to hosts linked against the same build's native
  objects (the symbol-localized `libcetta-*.a` hides the internals the
  borrowed pointers are used with). The header says so.

## Alternatives considered

- Duplicating `struct cetta_atom` in the host: works today, fragile.
- A public "seeded rematch" embedding entry point taking handles: wider API
  surface and a handle per candidate; the kernel's hot loop would pay for it.

## Tested

`make test` in cettapack (differential tests against generic CeTTa, 19 items)
on a build of `4c285a97` + this patch, BUILD=mork, gcc 11.4.
