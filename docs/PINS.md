# Pins

Every external input is pinned by commit. Nothing from CeTTa, MORK or PathMap is
copied into this repository: they are git submodules, and the build works on a
scratch clone (`build/cetta-src`), never on the submodule's history.

| What | Path | Source | Commit | Why this one |
|---|---|---|---|---|
| CeTTa | `cetta/` | https://github.com/zariuq/CeTTa | `a9b3a81186529fd99649e437809bdb6f58fce2fd` | main HEAD on 2026-11-17 ("Improve native PeTTa matching, iteration and memory use", committed 2026-11-17 00:59 UTC), task t-2026-11-17-cettapack-m1-numbers. M1 used `4c285a97`, which is no longer on main after Zar's Nov 6 rewrite (the same tree is `4b1245a9` there); the M1 baseline rerun uses M1's tree and pin unchanged |
| MORK | `deps/MORK/` | https://github.com/trueagi-io/MORK | `ed57c6716d8c510296fb5fbb8be6fbfe2df241d7` | the revision CeTTa's README names for its bridge lockfile (README.md:384 at the CeTTa pin) |
| Design note | `docs/cetta-guarded-supercompilation-2026-07-06.pdf` | Ben Goertzel, *CeTTa-Hosted Guarded Supercompilation for MeTTa*, 2026-07-06 (Google Drive file 1DpdolqeaCLPXCyZNbX6yZypPn0-BO8gP) | sha256 `757a25390f2c6acb64f077455246e17cba592a9c75bf97dbb434dc677ebd0cb2` (205,272 bytes) | the M1 design; committed in this branch's first commit |
| PathMap | `deps/PathMap/` | https://github.com/Adam-Vandervorst/PathMap | `ef0259152867ece8e85888acc49c238f264a0330` | the revision CeTTa's README names (README.md:385 at the CeTTa pin) |

Patches applied to the scratch copy of CeTTa, in order:

| Patch | Applies to | Purpose |
|---|---|---|
| `upstream/cetta/0001-embedding-in-process-native-borrowing-for-compiled-k.patch` | CeTTa `a9b3a811` (rebased from `4c285a97`; only the `#include` context in `src/embedding.c` changed) | three accessors (`include/cetta_embed_native.h`) so a callback can borrow the runtime Atom/Space behind embedding handles and return an engine-owned copy; see `upstream/cetta/PR_DESCRIPTION.md` |
| `upstream/cetta/0002-mork-bridge-bulk-prefix-lookup-mork_space_prefix_val.patch` | CeTTa `a9b3a811` (+0001, independent of it) | `mork_space_prefix_vals` / `cetta_mork_bridge_space_prefix_vals`: every PathMap key below a prefix in one bridge call; see `upstream/cetta/PR_DESCRIPTION-0002.md` |
| `upstream/cetta/0003-eval-match-rows-direct-entry-to-the-conjunction-matcher.patch` | CeTTa `a9b3a811` (+0001, +0002; independent of both) | `(match-rows <space> <pattern> <row>)`: CeTTa's conjunction matcher, rows returned as data; probe 1 of docs/NEXT_STEP_PROBES.md; see `upstream/cetta/PR_DESCRIPTION-0003.md` |
| `upstream/cetta/0004-match-rows-row-hooks-compiled-code-inside-the-matcher.patch` | CeTTa `a9b3a811` (+0001, +0002, +0003; extends 0003's match-rows and 0001's native header) | `(match-rows <space> <pattern> <row> (<hook> <arg> ...) <residual>)`: the matcher's rows handed to a host row hook (`cetta_embed_native_rows_hook_register`), the residual evaluated when the hook refuses; fnspec 3 item 3 (docs/FNSPEC_3.md); see `upstream/cetta/PR_DESCRIPTION-0004.md` |

The patch series digest is part of every pack's profile digest
(`tools/cpkc.py --print-profile-components`), so a pack compiled against one
series fails its guard under another.

Other inputs:

| What | Version | Notes |
|---|---|---|
| gcc | 11.4.0 (Ubuntu 22.04) | used for CeTTa and for this repo |
| glibc | 2.35 | see docs/BUILD.md for the DT_RELR workaround |
| Rust | nightly (rustup default on the host) | builds CeTTa's bridge crates against the pinned MORK/PathMap |
| libgmp headers | Ubuntu `libgmp-dev 2:6.2.1+dfsg-3ubuntu1`, unpacked user-locally | runtime `libgmp.so.10` is the system's |
| hyperon (HE) | 0.2.10 from PyPI (user site-packages) | comparison only (A.9 item 16, benchmark) |
| Approximate-CettaPacks note | not committed | context only, per the order (sha256 `98deb3542e01f1b356f98ddf1c9bc910ecf3c0ae2d9050953a9cc0f6173b998b`) |
| KB | `bench/gen_kb.py --seed 20261106` | 128,576 atoms, sha256 `8097f2f4ad2990cdf096cdcf428cf719397f4695536b6f51416fe9e3d5439f5c` (the same bytes for every `--query-kind`) |
