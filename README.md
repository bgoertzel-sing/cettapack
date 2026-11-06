# cettapack

CeTTa-hosted guarded supercompilation (CettaPacks): match kernels, manifests,
guards, benchmarks. CeTTa, MORK and PathMap are pinned submodules
(docs/PINS.md); nothing of theirs is copied here.

## Milestone 1: a guarded hot match kernel

One fixed match site,

```metta
(= (cpk-site-target-expressed $space $rs)
   (match $space (, (target $rs $gene) (expressed $gene $tissue)) ($gene $tissue)))
```

is compiled (`tools/cpkc.py`) to a C kernel plan plus a CettaPack manifest. At
run time the site is a CeTTa callable. Each call is guarded; when the guard
says VALID the kernel narrows candidates with PathMap (through CeTTa's MORK
bridge), decides every binding with CeTTa's native seeded rematch on every
source record, and returns generic CeTTa's answer bag with its multiplicities.
Any guard failure runs the residual, generic CeTTa's own `match`.

| | |
|---|---|
| Kernel runtime | `src/cpk_kernel.c` (interface `src/cpk.h`) |
| Loader, guard, callable, fallback | `src/cpk_pack.c` |
| Compiler | `tools/cpkc.py` -> `packs/target_expressed/{kernel_plan.c,manifest.json}` |
| Differential tests, benchmark driver | `src/cpk_host.c` |
| Certificate skeleton | `tools/emit_certificate.py` -> `packs/target_expressed/certificate.json` |
| KB generator (seeded) | `bench/gen_kb.py` |
| CeTTa patch + PR text | `upstream/cetta/` |
| Design and adaptations | `docs/DESIGN.md` |
| Build | `docs/BUILD.md` |
| Benchmark results | `docs/BENCHMARK.md`, raw logs in `evidence/bench/` |
| Test output | `evidence/tests/` |

```bash
git submodule update --init
scripts/build_cetta.sh     # CeTTa BUILD=mork with the MORK/PathMap bridge, patched scratch copy
make test                  # one command: all differential tests + certificate
bench/run_bench.sh         # benchmark
```
