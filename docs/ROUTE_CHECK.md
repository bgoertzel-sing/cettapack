# Route check: which match route each build takes, and did the baselines run on native rows?

Task t-2026-11-17-cettapack-route-check (andy, Work msg 744). Branch hive/t-2026-11-17-cettapack-route-check,
base hive/t-2026-11-17-cettapack-m1-numbers at dd3c30a (not merged to main; main is d2df1cc). Evidence:
evidence/route-check-2026-11-17/. Host pop-os (i7-1165G7, shared, load average 2.5-4), 2026-11-17.

## Builds compared

| label | cettapack source | CeTTa | patches | benchmarked binary |
|---|---|---|---|---|
| 4c (M1) | d2df1cc | 4c285a97 | M1's 0001 | wt/m1-baseline/build/cettapack-host |
| a9 (M1-numbers) | dd3c30a (code = 4bd2687) | a9b3a811 | 0001 rebased + 0002 | wt/m1-numbers/build/cettapack-host-<site> |

The order names 4b1245a9 as the old build. 4b1245a9 is a CeTTa commit with the same subject and timestamp as
4c285a97 ("WIP: make pre-main shareable and preserve benchmark evidence"); they differ only in
docs/petta_compatibility.txt (143 lines added). M1 and the 40 us figure were measured on 4c285a97, so that is the
pin used here. Neither has the new decline in cetta_native_query_run (checked: the old native_query.c has no
space_match_backend_index_forced_without_rows; a9b3a811 has it at native_query.c:2504-2512).

## Method (observed at runtime)

1. **Probe.** bench/route-check/route_probe.inc is inserted, by bench/route-check/insert_probe.py, into a *copy*
   of each tree's src/cpk_host.c (the benchmarked hosts and Zar's repo are untouched). With CPK_ROUTE_PROBE=1
   it prints, outside the timed loop (after load, before the timed batches, after them):
   - the space's match backend, read from CeTTa's own Space (cetta_embed_native_space -> match_backend.kind);
   - native row storage: Space.native.len (rows) and dense_facts (facts held in dense relations);
   - for a PathMap backend, whether the native shadow is synced; CETTA_PATHMAP_QUERY_INDEX;
   - on a9, CeTTa's own space_match_backend_index_forced_without_rows(space), the exact condition under which
     a9's cetta_native_query_run now declines;
   - CeTTa's runtime counters over exactly the timed batches. CeTTa's (reset-runtime-stats!) and
     (runtime-stats!) are not admitted in the embedded HE profile (they come back unevaluated), so the probe
     registers two 0-ary host callbacks that reset/enable and snapshot the engine's stats owner from inside
     the engine's scope.
2. **Builds.** bench/route-check/build_cetta_variants.sh rebuilds copies of the two benchmark CeTTa trees with
   ENABLE_RUNTIME_STATS=1 (separate *.runtime-stats.o objects; the stats-off objects stay as benchmarked).
   bench/route-check/build_probe_hosts.sh links the probe hosts against both the stats-off objects (backend
   facts on exactly the benchmarked CeTTa code) and the stats-on objects (counters). The pack profile defines
   come from the benchmarked tree (a first probe build with wrong defines made kernel modes fall back with
   "profile mismatch"; it was caught by kernel_calls=0 and rebuilt; those outputs were discarded).
3. **Inputs.** The benchmark's own KB/queries (wt/m1-numbers/build, KB sha256 8097f2f4..., 128,576 atoms).
   bench/route-check/run_probe.sh runs every mode of every site (a9: 4 sites x 5 modes; 4c: M1's site x 4 modes).
   Answers per batch equal the benchmark's in every probe run (1653 / 7377 / 173693 / 9835), and kernel modes
   report kernel_calls = calls, fallbacks 0, reason "ok".

## 1. Route and native-row status (every mode/site, both builds)

Counters are per query over 5 batches (REPEAT=5). Full tables: evidence/route-check-2026-11-17/routes-*.md;
raw probe output: probe-{a9,4c}-stats{off,on}/<site>-<mode>.probe.

| build | site | mode | backend | rows + dense facts before the timed batches | forced PathMap without rows | native-query-engine admissions / query | native-backend matcher probes / query (match-native-probe) | CeTTa PathMap/MORK counters |
|---|---|---|---|---|---|---|---|---|
| 4c | target_expressed (M1) | generic | native | 55,315 + 73,261 | n/a (no such check), env unset | 0 | 2.76 | none |
| 4c | target_expressed | hosted-generic | native | 82,177 + 46,399 | n/a, env unset | 0 | 2.76 | none |
| 4c | target_expressed | kernel | native | 128,576 + 0 | n/a, env unset | 0 | 0 (kernel) | none |
| 4c | target_expressed | fallback | native | 128,576 + 0 | n/a, env unset | 0 | 2.76 | none |
| a9 | target_expressed | generic | native | 55,315 + 73,261 | no, env unset | 0 | 2.76 | none |
| a9 | target_expressed | hosted-generic | native | 82,177 + 46,399 | no | 0 | 2.76 | none |
| a9 | target_expressed | kernel, kernel-cursor | native | 128,576 + 0 | no | 0 | 0 (kernel) | none |
| a9 | target_expressed | fallback | native | 128,576 + 0 | no | 0 | 2.76 | none |
| a9 | gene_variants | generic / hosted-generic / fallback | native | 55,315+73,261 / 82,177+46,399 / 128,576+0 | no | 0 | 8.25 | none |
| a9 | gene_variants | kernel, kernel-cursor | native | 128,576 + 0 | no | 0 | 0 | none |
| a9 | tissue_fanout | generic / hosted-generic / fallback | native | 55,315+73,261 / 82,177+46,399 / 128,576+0 | no | 0 | 574.3 | none |
| a9 | tissue_fanout | kernel, kernel-cursor | native | 128,576 + 0 | no | 0 | 0 | none |
| a9 | variant_interactor | generic / hosted-generic / fallback | native | 55,315+73,261 / 112,164+16,412 / 128,576+0 | no | 0 | 12.9 | none |
| a9 | variant_interactor | kernel, kernel-cursor | native | 128,576 + 0 | no | 0 | 0 | none |

Reading it:
- **Every baseline ran on CeTTa's normal space with native rows.** The backend is CeTTa's native backend in
  every mode on every site on both builds (stats-off probes on the benchmarked objects and stats-on probes agree).
  CETTA_PATHMAP_QUERY_INDEX is unset, and on a9 space_match_backend_index_forced_without_rows() is false, so the
  decline added between the two builds cannot fire for these spaces. CeTTa's PathMap and MORK counters
  (pathmap-*, mork-*) are zero in every mode.
- **The route, precisely.** generic, hosted-generic and fallback all answer through the native backend's
  candidate matcher (match-native-probe / match-position-index-lookup, one per factor lookup; same counts in the
  three modes). cetta_native_query_run, the "native query engine" Godel's note is about, admits **no** query in
  any mode on either build (native-query-admission = 0, bindings-loop-call-native-query = 0). So the generic
  baseline did not use the native query engine on either build, and did not use PathMap either.
- The kernel modes narrow through cettapack's own PathMap/MORK index (68,873 groups / 74,399 rows on M1's site,
  built through the MORK bridge, not a CeTTa space backend), then rematch in CeTTa; they make no
  match-native-probe calls.
- Storage form differs by mode, all native: CeTTa keeps 73,261 facts in dense relations after load; the first
  generic match moves some back to rows (55,315+73,261 -> 82,177+46,399; hosted-generic does this during its
  untimed warm-up call), and the kernel's index build (also run, untimed, in fallback mode) moves all of them to
  rows (128,576+0). This is the same on both builds.

## 2. Plain match 40 -> 46 us

**Same route, same work, and the gap does not reproduce.** Two pieces of evidence:

1. **Counters (stats builds, M1's site, generic, per query):** every counter is identical between 4c and a9
   except match-position-index-key-probe (256.4 vs 254.9). Same matcher calls (2.76), candidates (10.03),
   unifications (10.03), bindings operations, answer transport (48.4 nodes, 12,677 metadata bytes) and
   term-universe traffic. So there is no route change and no extra work per query on a9
   (evidence/route-check-2026-11-17/counters-main-generic-4c-vs-a9.md).
2. **Stats-off re-time** with the benchmarked binaries themselves (sha256 60132427... and 8f83bd94...),
   identical flags (bench --mode generic --repeat 5), the same KB/queries and the same answer consumption
   (both hosts count results with cetta_embed_result_count in the identical loop), RUNS x REPEAT=5 on the same
   host, interleaved run by run (bench/route-check/retime.sh; and the full benchmark suite order via
   bench/run_bench.sh):

| session | runs | a9 generic us | 4c (M1) generic us | a9/4c paired median (range) | a9 slower |
|---|---|---|---|---|---|
| M1-numbers benchmark (the 46.2 vs 39.8 figures) | 7 | 46.2 | 39.8 | 1.090 (0.957-1.228) | 6/7 |
| retime-1: 4c then a9 in each run | 7 | 45.2 | 43.2 | 1.043 (1.031-1.280) | 7/7 |
| retime-2: a9 then 4c | 7 | 41.0 | 42.1 | 0.985 (0.926-1.105) | 3/7 |
| retime-3: order alternating | 14 | 43.5 | 44.1 | 0.994 (0.542-1.821) | 7/14 |
| rebench: benchmark suite order repeated | 7 | 44.4 | 42.8 | 1.038 (0.649-1.153) | 5/7 |
| all 35 reruns pooled | 35 | | | 1.031 | 22/35 |

hosted-generic (CeTTa's matcher called from C, no evaluator around it) is the same or slightly faster on a9
(paired medians 0.97, 0.93, 0.98).

Answer: the 40 -> 46 us change is not a route change. On the same route with the same counted work, the
reruns put a9 at 0-4% slower than 4c (pooled paired median +3%, a9 slower in 22 of 35 runs, and the sign flips
with process order), which is inside this shared host's run-to-run noise. The 14-16% gap of the M1-numbers
benchmark session did not reproduce; most of it was that session's noise. A real per-operation difference of a
few percent cannot be ruled out, and with identical counters it cannot be attributed further from runtime stats.

## 3. Corrected rows

None needed: no baseline ran on a PathMap-only route. docs/BENCHMARK.md's generic and hosted-generic rows were
measured on native rows. One correction to wording only: the M1-numbers summary's "the pin move made generic
CeTTa about 14% slower (46.2 vs 39.8 us)" is not supported by these reruns (0-4%, not distinguishable from noise).

## Limits

- Counters come from ENABLE_RUNTIME_STATS=1 builds of the same CeTTa trees; the backend facts were also read on
  the stats-off objects and agree. Stats builds could in principle take other paths; the stats-on and stats-off
  probes agree on backend, storage form and answers.
- The probe reads the backend and row storage before and after the timed batches, not per query; the counters
  cover every timed query.
- 4c was probed on M1's site only (M1's tree has only that pack); the option-B sites exist on a9 only.
- Shared host, load average 2.5-4 throughout; the ranges above show the noise.
- The a9 stats build's MORK bridge archive got a different file hash (libcetta_space_bridge...fecc1a8b8015.a vs
  ...a92fcc18e03d.a in the stats-off build of the same tree); it is only linked into the stats-on probe hosts,
  whose CeTTa PathMap/MORK counters are zero anyway.
