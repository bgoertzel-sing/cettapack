#!/usr/bin/env python3
"""cetta-internals P: end-to-end estimates for the ranked evaluator changes (Amdahl over measured shares).

  estimates.py EVPROF.json ABLATIONS.json [--proto PROTO.json] [--md OUT.md] [--json OUT.json]

EVPROF.json: bench/p/evprof_report.py --json (per workload: each timed function's corrected self share, median over
runs).  ABLATIONS.json: bench/p/ablation_report.py --json (end-to-end user-time ratios of the zero-code calibration
runs).  PROTO.json (optional): the same for the prototype build against its base.

Each change names the timed functions whose self time it changes (the time it touches, measured) and, per group of
functions, the fraction of that time it removes (assumed unless a calibration run measured it; every fraction says
which).  Saved share s = sum(share * fraction); end-to-end speed-up = 1 / (1 - s).  The changes touch disjoint
functions, so the combined saving is the sum of the savings (no double counting); interactions (e.g. fewer
allocations because fewer decisions are recomputed) are ignored, which errs low for the combination.
"""
import argparse
import json

WORKLOADS = ["w1-examples", "w2-wmtm", "w3-benchgen"]

CHANGES = [
    {
        "id": "C1",
        "name": "Call-preparation decisions cached per call site and invalidated per head",
        "groups": [
            {"label": "per-call admission, safety and dispatch-mode checks (validation of 5-10 revision fields, "
                      "table-safety scans, specializer admission, arity lookups through Prolog)",
             "fraction": 0.6, "basis": "assumed",
             "sites": ["petta_eval_machine_builtin_equations", "petta_machine_builtin_equations_mode",
                       "petta_eval_open_program_acquire", "petta_eval_open_program_lookup_locked",
                       "petta_eval_open_foreign_reads_current", "cetta_open_equation_program_host_reads_current",
                       "petta_program_refresh_callability", "petta_program_relation_table_safe",
                       "petta_table_safety_scan_relation", "petta_eval_machine_tabled_relation_admissible",
                       "petta_specializer_query_value_execution_admission",
                       "petta_program_candidate_snapshot_lease_profiled", "petta_eval_machine_candidate_snapshot_lease",
                       "petta_program_head_is_intrinsic", "petta_relation_specialization_relevance",
                       "petta_eval_machine_space_query_admission_policy", "petta_eval_relation_admission_entry",
                       "petta_eval_open_relation_admitted", "eval_space_mutation_contract_kind",
                       "space_query_effect_for_head_analyzed", "get_annotated_types",
                       "petta_specializer_prepare_call", "petta_machine_try_ready_relation_invocation",
                       "petta_eval_machine_open_relation_declined", "petta_machine_start_equation_choice",
                       "petta_machine_extension_named_arity_using", "petta_machine_named_arity",
                       "petta_machine_source_named_arity", "petta_machine_named_partial_decision",
                       "petta_eval_machine_classify_host"]},
        ],
    },
    {
        "id": "C2",
        "name": "Prepared-call path: indexed cache, per-head invalidation, no repeated compile of unchanged heads",
        "groups": [
            {"label": "cache lookups and refusal checks (list walks)", "fraction": 0.85, "basis": "proto",
             "sites": ["prepared_pure_program_cache_has_decline_head", "prepared_pure_closed_invocation_plan",
                       "petta_eval_machine_prepared_refused", "prepared_pure_closed_invocation_try"]},
            {"label": "recompiling programs and refusals after unrelated program changes", "fraction": 0.5,
             "basis": "assumed",
             "sites": ["prepared_pure_program_compile_closed_mode", "prepared_pure_compile_pending_heads",
                       "oem_compile_certificate"]},
        ],
    },
    {
        "id": "C3",
        "name": "Allocation: pooled/arena allocation of per-call machine records (cursors, choices, frames)",
        "groups": [
            {"label": "malloc/calloc/realloc/free and arena growth", "fraction": 0.5, "basis": "assumed",
             "sites": ["malloc", "calloc", "realloc", "free", "arena_grow", "petta_answer_choice_release",
                       "petta_choice_release"]},
        ],
    },
    {
        "id": "C4",
        "name": "Copying: share ground and settled subterms instead of re-instantiating them",
        "groups": [
            {"label": "bindings application, deep copies, frame instantiation", "fraction": 0.35, "basis": "assumed",
             "sites": ["bindings_apply_value", "bindings_apply_fold", "atom_deep_copy_session_copy",
                       "cetta_instantiate_frame_terms", "oem_instantiate", "petta_fresh_variable",
                       "bindings_project_reachable", "term_universe_copy_atom_impl", "petta_copy_binding_atoms"]},
        ],
    },
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("evprof")
    ap.add_argument("--fractions", help="JSON {change id: {group index: fraction}} overriding the defaults "
                                        "(e.g. calibrated by a measurement)")
    ap.add_argument("--md")
    ap.add_argument("--json")
    a = ap.parse_args()
    ev = json.load(open(a.evprof))
    over = json.load(open(a.fractions)) if a.fractions else {}
    res = {"changes": [], "combined": {}}
    seen = set()
    lines = ["| change | group | basis | fraction removed | " +
             " | ".join("%s touched / saved" % w for w in WORKLOADS) + " |",
             "|---|---|---|---|" + "---|" * len(WORKLOADS)]
    totals = {w: 0.0 for w in WORKLOADS}
    for ch in CHANGES:
        cres = {"id": ch["id"], "name": ch["name"], "groups": [], "saved": {}, "speedup": {}, "touched": {}}
        for gi, g in enumerate(ch["groups"]):
            frac = over.get(ch["id"], {}).get(str(gi), {}).get("fraction", g["fraction"])
            basis = over.get(ch["id"], {}).get(str(gi), {}).get("basis", g["basis"])
            gres = {"label": g["label"], "fraction": frac, "basis": basis, "touched": {}, "saved": {}}
            for s in g["sites"]:
                assert s not in seen, s
                seen.add(s)
            cells = []
            for w in WORKLOADS:
                sites = ev["workloads"][w]["sites"]
                t = sum(sites[s]["self_share"] for s in g["sites"] if s in sites)
                gres["touched"][w] = t
                gres["saved"][w] = t * frac
                cres["touched"][w] = cres["touched"].get(w, 0.0) + t
                cres["saved"][w] = cres["saved"].get(w, 0.0) + t * frac
                cells.append("%.1f%% / %.1f%%" % (100 * t, 100 * t * frac))
            lines.append("| %s | %s | %s | %.2f | %s |" % (ch["id"], g["label"], basis, frac, " | ".join(cells)))
            cres["groups"].append(gres)
        for w in WORKLOADS:
            cres["speedup"][w] = 1.0 / (1.0 - cres["saved"][w])
            totals[w] += cres["saved"][w]
        lines.append("| **%s total** | %s | | | %s |" % (ch["id"], ch["name"], " | ".join(
            "%.1f%% / **%.1f%% (%.2fx)**" % (100 * cres["touched"][w], 100 * cres["saved"][w], cres["speedup"][w])
            for w in WORKLOADS)))
        res["changes"].append(cres)
    lines.append("| **all four** | | | | %s |" % " | ".join(
        "**%.1f%% (%.2fx)**" % (100 * totals[w], 1 / (1 - totals[w])) for w in WORKLOADS))
    res["combined"] = {w: {"saved": totals[w], "speedup": 1 / (1 - totals[w])} for w in WORKLOADS}
    text = "\n".join(lines) + "\n"
    if a.md:
        open(a.md, "w").write(text)
    else:
        print(text)
    if a.json:
        json.dump(res, open(a.json, "w"), indent=1)


if __name__ == "__main__":
    main()
