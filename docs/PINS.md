# Pins

Every external input is pinned by commit. Nothing from CeTTa, MORK or PathMap is
copied into this repository: they are git submodules, and the build works on a
scratch clone (`build/cetta-src`), never on the submodule's history.

| What | Path | Source | Commit | Why this one |
|---|---|---|---|---|
| CeTTa | `cetta/` | https://github.com/zariuq/CeTTa | `4c285a97fd8c60e3c3fccd5ccd3b3e5e0ebccb4c` | main HEAD when the task started (2026-11-06 05:10 UTC; `git ls-remote` HEAD still equal at 05:2x UTC) |
| MORK | `deps/MORK/` | https://github.com/trueagi-io/MORK | `ed57c6716d8c510296fb5fbb8be6fbfe2df241d7` | the revision CeTTa's README names for its bridge lockfile (README.md:384 at the CeTTa pin) |
| PathMap | `deps/PathMap/` | https://github.com/Adam-Vandervorst/PathMap | `ef0259152867ece8e85888acc49c238f264a0330` | the revision CeTTa's README names (README.md:385 at the CeTTa pin) |

Patches applied to the scratch copy of CeTTa, in order:

| Patch | Applies to | Purpose |
|---|---|---|
| `upstream/cetta/0001-embedding-in-process-native-borrowing-for-compiled-k.patch` | CeTTa `4c285a97` | three accessors (`include/cetta_embed_native.h`) so a callback can borrow the runtime Atom/Space behind embedding handles and return an engine-owned copy; see `upstream/cetta/PR_DESCRIPTION.md` |

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
| hyperon (HE) | 0.2.10 from PyPI (user site-packages) | comparison only |
| KB | `bench/gen_kb.py --seed 20261106` | 128,576 atoms, sha256 `8097f2f4ad2990cdf096cdcf428cf719397f4695536b6f51416fe9e3d5439f5c` |
