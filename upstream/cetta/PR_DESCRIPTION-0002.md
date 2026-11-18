# PR: mork bridge: bulk prefix lookup (mork_space_prefix_vals)

Patch: `0002-mork-bridge-bulk-prefix-lookup-mork_space_prefix_val.patch`
Applies to: zariuq/CeTTa `a9b3a81186529fd99649e437809bdb6f58fce2fd` after 0001 (`git am`; it does not
depend on 0001 and also applies alone).
Size: 3 files, +132 (one Rust export, its C wrapper in the static and the
dlopen bridge paths, one header declaration).

## What it adds

```c
/* src/mork_space_bridge_runtime.h */
bool cetta_mork_bridge_space_prefix_vals(const CettaMorkSpaceHandle *space,
                                         const uint8_t *prefix, size_t prefix_len,
                                         uint64_t max_rows,
                                         uint8_t **out_packet, size_t *out_len,
                                         uint64_t *out_rows);
```

backed by `mork_space_prefix_vals` in `rust/cetta-space-bridge/src/lib.rs`. One call
descends the space's PathMap trie to `prefix` once (`read_zipper_at_path`) and
returns every key at or below it: a packet of (big-endian u32 length, key
bytes after the prefix) records in trie order, and the key count. An absent
prefix gives an empty packet. `max_rows` (0 = unbounded) turns "more keys than
expected" into an error rather than a silent truncation. The packet is freed
with `cetta_mork_bridge_bytes_free`, like every other bridge buffer.

## Why

The existing way to read the keys below a prefix is the cursor API:
`cursor_reset`, one `cursor_descend_byte` per prefix byte, then `cursor_next_val` +
`cursor_path_bytes` per key. Each of those is an FFI call, and each Rust-side
cursor call rebuilds a read zipper and re-descends from the root to the
cursor's saved path; `cursor_path_bytes` also allocates a fresh buffer per key.
For short, selective lookups (a compiled match kernel narrowing a join on a
ground prefix: about 20 prefix bytes and a handful of keys) that is dozens of
FFI round trips and root descents per lookup. cettapack measured 8.9 us per
narrowing call that way on CeTTa `4c285a97`. The bulk call does one descent and
one walk of the subtree. Measured effect: see "Tested".

## What it does not do

- It changes no existing function. The space is only read.
- Keys are trie paths, so the result is the same for every storage mode of
  the space (raw or counted); it does not report multiplicities.
- In the dlopen bridge path the symbol is resolved as optional: an older
  bridge library without it makes the wrapper fail with a clear error
  instead of failing to load.

## Alternatives considered

- Batching on the cursor (`cursor_next_expr_rows`): already exists, but returns
  decoded expressions of the whole remaining trie from the cursor position, not
  the raw keys under one prefix, and still needs the per-byte descent first.
- Keeping a read zipper alive inside the cursor between calls: removes the
  re-descent, not the FFI round trips, and changes the cursor's lifetime rules.

## Tested

cettapack `make test` (M1's 23 differential items plus the HE comparison, and
the new per-site differential bag tests on four sites, 36 items) on a
`BUILD=mork` build of `a9b3a811` + 0001 + this patch, gcc 11.4, Ubuntu 22.04; the
kernel's narrowing uses this call. Narrowing per kernel call, same binary,
this call vs the per-byte cursor calls: 2.3 vs 8.3 us on a selective lookup
(8.7 keys), 7.3 vs 23.5 us (38 keys), 0.93 vs 2.42 ms (4,255 keys); 2.6x to
3.6x less (cettapack `docs/BENCHMARK.md`, RUNS=7 REPEAT=5).
