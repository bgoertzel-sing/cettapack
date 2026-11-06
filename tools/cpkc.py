#!/usr/bin/env python3
"""cpkc: compile one CettaPack M1 match site into a kernel plan and a manifest.

Input: a site source holding one equation of the M1 shape

    (= (<site> $space $in ...) (match $space (, f1 ... fn) <template>))

where every factor fi is a flat expression of symbols and variables. The
compiler chooses a join order (most ground fields first, then source order),
emits packs/<site>/kernel_plan.c (a static CpkPlan for src/cpk_kernel.c) and
packs/<site>/manifest.json (CettaPack manifest, v0.2 shape).

Hashes (all SHA-256, hex):
  structure: cpk-struct-v1 over canonical text (variables renamed $_0, $_1 ...
             by first occurrence in the hashed unit; one space between children)
  profile:   cpk-profile-v1 over the CeTTa pin, the applied patch series, the
             BUILD mode and the engine language
  langdef:   cpk-tree-v1 over CeTTa's langdef/<tree> directory
  schema:    cpk-schema-v1 over the factor relations (head/arity:field kinds)
"""
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


# ---- s-expressions -----------------------------------------------------------

def tokenize(text):
    text = re.sub(r";[^\n]*", "", text)
    return re.findall(r"\(|\)|[^\s()]+", text)


def parse(text):
    toks = tokenize(text)
    pos = 0

    def rec():
        nonlocal pos
        t = toks[pos]
        pos += 1
        if t == "(":
            out = []
            while toks[pos] != ")":
                out.append(rec())
            pos += 1
            return out
        if t == ")":
            raise ValueError("unbalanced )")
        return t

    forms = []
    while pos < len(toks):
        forms.append(rec())
    return forms


def is_var(a):
    return isinstance(a, str) and a.startswith("$")


def canon(a, names):
    if isinstance(a, list):
        return "(" + " ".join(canon(c, names) for c in a) + ")"
    if is_var(a):
        if a not in names:
            names[a] = len(names)
        return f"$_{names[a]}"
    return a


def struct_hash(a):
    return hashlib.sha256(("cpk-struct-v1\n" + canon(a, {})).encode()).hexdigest()


def tagged_hash(tag, text):
    """sha256(tag + newline + text): one domain tag per identity field (A.5)."""
    return hashlib.sha256((tag + "\n" + text).encode()).hexdigest()


def first_vars(a, out):
    if isinstance(a, list):
        for c in a:
            first_vars(c, out)
    elif is_var(a) and a not in out:
        out.append(a)
    return out


# ---- digests -----------------------------------------------------------------

def tree_digest(root):
    h = hashlib.sha256(b"cpk-tree-v1\n")
    files = []
    for d, _, fs in os.walk(root):
        for f in fs:
            full = os.path.join(d, f)
            if os.path.isfile(full):
                files.append(os.path.relpath(full, root))
    for rel in sorted(files, key=lambda s: s.encode()):
        with open(os.path.join(root, rel), "rb") as fh:
            ch = hashlib.sha256(fh.read()).hexdigest()
        h.update(rel.encode() + b"\0" + ch.encode() + b"\n")
    return h.hexdigest()


def patches_digest(patch_dir):
    h = hashlib.sha256(b"cpk-patches-v1\n")
    for name in sorted(os.listdir(patch_dir)):
        if name.endswith(".patch"):
            with open(os.path.join(patch_dir, name), "rb") as fh:
                data = fh.read()
            h.update(name.encode() + b"\0" + hashlib.sha256(data).hexdigest().encode() + b"\n")
    return h.hexdigest()


def cetta_commit(cetta_dir):
    return subprocess.check_output(["git", "-C", cetta_dir, "rev-parse", "HEAD"], text=True).strip()


def profile(commit, patches, build, language):
    text = f"cpk-profile-v1\ncetta={commit}\npatches={patches}\nbuild={build}\nlanguage={language}\n"
    return hashlib.sha256(text.encode()).hexdigest(), text


def schema_digest(factors):
    lines = ["cpk-schema-v1\n"]
    for f in factors:
        lines.append(f"{f[0]}/{len(f)}:" + ",".join("sym" for _ in f[1:]) + "\n")
    return hashlib.sha256("".join(lines).encode()).hexdigest()


# ---- compile -----------------------------------------------------------------

def compile_site(src_path):
    with open(src_path) as fh:
        forms = parse(fh.read())
    if len(forms) != 1:
        raise SystemExit("site source must hold exactly one equation")
    region = forms[0]
    ok = (isinstance(region, list) and len(region) == 3 and region[0] == "="
          and isinstance(region[1], list) and isinstance(region[2], list)
          and len(region[2]) == 4 and region[2][0] == "match")
    if not ok:
        raise SystemExit("site is not (= (<site> ...) (match $space (, ...) <template>))")
    lhs, body = region[1], region[2]
    site, params = lhs[0], lhs[1:]
    space_var, conj, template = body[1], body[2], body[3]

    def has_match(x):
        return isinstance(x, list) and (x[:1] == ["match"] or any(has_match(y) for y in x))
    if has_match(conj) or has_match(template):
        raise SystemExit("refused: nested match / cross-space join is not supported in M1 (A.9 item 8)")
    if not (isinstance(conj, list) and conj and conj[0] == ","):
        raise SystemExit("match pattern must be a conjunction (, f1 ... fn)")
    factors = conj[1:]
    if len(factors) < 2:
        raise SystemExit("M1 kernels are compiled for conjunctions of two or more patterns")
    if not is_var(space_var) or space_var not in params:
        raise SystemExit("the matched space must be a parameter of the site")
    for f in factors:
        if not (isinstance(f, list) and f and not is_var(f[0]) and not isinstance(f[0], list)):
            raise SystemExit(f"factor {f} must be flat with a symbol head")
        for x in f:
            if isinstance(x, list):
                raise SystemExit(f"factor {f} is not flat")
    slots = first_vars(region, [])
    slot = {v: i for i, v in enumerate(slots)}

    # join order: greedy, most ground fields (constants + already bound) first
    bound = {p for p in params if p != space_var}
    order, left = [], list(range(len(factors)))
    while left:
        def score(i):
            f = factors[i]
            return (sum(1 for x in f[1:] if not is_var(x) or x in bound), -i)
        best = max(left, key=score)
        order.append(best)
        left.remove(best)
        bound |= {x for x in factors[best] if is_var(x)}

    return {
        "region": region, "site": site, "params": params, "space_var": space_var,
        "conj": conj, "template": template, "factors": factors, "slots": slots,
        "slot": slot, "order": order,
    }


def c_str(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def emit_plan_c(c, src_rel):
    ident = re.sub(r"[^A-Za-z0-9_]", "_", c["site"])
    lines = [
        "/* Generated by tools/cpkc.py from " + src_rel + ". Do not edit. */",
        '#include "cpk.h"',
        "",
        f"const CpkPlan cpk_plan_{ident} = {{",
        f"    .site = {c_str(c['site'])},",
        f"    .nslots = {len(c['slots'])},",
        "    .slot_names = {" + ", ".join(c_str(v) for v in c["slots"]) + "},",
        f"    .nfactors = {len(c['factors'])},",
        "    .factors = {",
    ]
    for ci in c["order"]:
        f = c["factors"][ci]
        fields = []
        for x in f:
            if is_var(x):
                fields.append(f"{{CPK_FIELD_SLOT, {c['slot'][x]}, 0}}")
            else:
                fields.append(f"{{CPK_FIELD_CONST, 0, {c_str(x)}}}")
        lines.append(f"        {{ .conj_index = {ci}, .arity = {len(f)}, .fields = {{{', '.join(fields)}}} }},")
    lines.append("    },")
    lines.append(f"    .space_input = {c['params'].index(c['space_var'])},")
    lines.append(f"    .ninputs = {len(c['params'])},")
    ins = [(-1 if p == c["space_var"] else c["slot"][p]) for p in c["params"]]
    lines.append("    .input_slot = {" + ", ".join(str(x) for x in ins) + "},")
    lines.append(f"    .region_text = {c_str(canon(c['region'], {}))},")
    lines.append("};")
    return "\n".join(lines) + "\n", ident


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("site_source", nargs="?")
    ap.add_argument("--out-dir")
    ap.add_argument("--cetta", default=os.path.join(ROOT, "cetta"))
    ap.add_argument("--patches", default=os.path.join(ROOT, "upstream", "cetta"))
    ap.add_argument("--build", default="mork")
    ap.add_argument("--language", default="he")
    ap.add_argument("--langdef-tree", default="he")
    ap.add_argument("--print-profile-components", action="store_true")
    a = ap.parse_args(argv)

    commit = cetta_commit(a.cetta)
    pdig = patches_digest(a.patches)
    if a.print_profile_components:
        print(f"CPK_CETTA_COMMIT={commit}")
        print(f"CPK_PATCHES_SHA256={pdig}")
        print(f"CPK_BUILD_MODE={a.build}")
        return 0
    if not a.site_source or not a.out_dir:
        ap.error("site_source and --out-dir are required")

    c = compile_site(a.site_source)
    os.makedirs(a.out_dir, exist_ok=True)
    src_rel = os.path.relpath(os.path.abspath(a.site_source), ROOT)
    plan_c, ident = emit_plan_c(c, src_rel)
    with open(os.path.join(a.out_dir, "kernel_plan.c"), "w") as fh:
        fh.write(plan_c)
    prof, prof_text = profile(commit, pdig, a.build, a.language)
    factors_conj = c["factors"]
    with open(a.site_source, "rb") as fh:
        source_bytes = fh.read()
    region_canon = canon(c["region"], {})
    schema_rel = schema_digest(factors_conj)
    op_schemas = sorted({f"{f[0]}/{len(f)}" for f in factors_conj})
    manifest = {
        # A.2 manifest v0.2. Values that M1 does not compute say so in place.
        "artifact_kind": "cettapack",
        "manifest_version": "0.2",
        "identity": {
            "source_span_hash": hashlib.sha256(b"cpk-span-v1\n" + source_bytes).hexdigest(),
            "normalized_region_hash": tagged_hash("cpk-normalized-v1", region_canon),
            "meta_expanded_region_hash": tagged_hash("cpk-meta-expanded-v1", region_canon),
            "match_pattern_hash": tagged_hash("cpk-pattern-v1", canon(c["conj"], {})),
            "langdef_pack_digest": tree_digest(os.path.join(a.cetta, "langdef", a.langdef_tree)),
            "profile_digest": prof,
            "operator_schema_hashes": [tagged_hash("cpk-opschema-v1", s) for s in op_schemas],
            "effect_policy_digest": tagged_hash("cpk-effects-v1", "Read(space:evidence, scope:snapshot)\nisolation=SnapshotIsolated"),
            "exactness_signature": "exact",
        },
        "effects": {
            "classes": ["Read(space:evidence, scope:snapshot)"],
            "isolation": "SnapshotIsolated",
        },
        "spaces": [{
            "role": "evidence",
            "space_id": f"input:{c['params'].index(c['space_var'])}",
            "schema_hash": schema_rel,
            "revision_policy": "append_only_epoch",
            "revision_or_epoch": "captured at instantiation (space revision and length)",
            "stats_signature": "not computed in M1 (REPLAN is a stub)",
            "multiplicity_source": "space_records",
            "pathmap_coverage_hash": tagged_hash("cpk-coverage-v1", "\n".join(f[0] + "/" + str(len(f)) for f in factors_conj)),
        }],
        "guards": {
            "fast_path": "dependency_vector_equal",
            "patch_path": "append_only_delta_under_budget (M1 stub: PATCHABLE falls back)",
            "replan_path": "stats_drift_over_threshold (M1 stub: never raised)",
            "invalid_path": "fallback_to_cetta",
        },
        "artifacts": {
            "native_so_hash": None,
            "native_plan_hash": hashlib.sha256(plan_c.encode()).hexdigest(),
            "pathmap_artifact_hashes": [],
            "mm2_fragment_hashes": [],
            "residual_metta_hash": hashlib.sha256(b"cpk-residual-v1\n" + source_bytes).hexdigest(),
            "transform_cert_hash": None,
        },
        "amortization": {
            "compile_cost_estimate_ms": 0, "expected_calls": 0, "expected_validity_window_ms": 0,
            "max_patch_cost_ms": 0, "max_stats_drift": 0.0, "max_append_delta_atoms": 4096,
        },
        # cettapack extensions (not in A.2): what the loader and tools need
        "x_cettapack": {
            "site": {"name": c["site"], "source": src_rel, "params": c["params"], "space_param": c["space_var"]},
            "profile_components": {"method": "cpk-profile-v1", "cetta": commit, "patches": pdig, "build": a.build, "language": a.language},
            "langdef_tree": a.langdef_tree,
            "template_hash": tagged_hash("cpk-template-v1", canon(c["template"], {})),
            "region_canonical": region_canon,
            "meta_expansion": "identity: the region holds no macros or meta-rules; the hash is recomputed from CeTTa's parse at load",
            "relations": [{"head": f[0], "arity": len(f), "fields": ["sym"] * (len(f) - 1)} for f in factors_conj],
            "kernel": {"plan_symbol": f"cpk_plan_{ident}", "plan_file": os.path.relpath(os.path.join(a.out_dir, "kernel_plan.c"), ROOT),
                       "runtime": "src/cpk_kernel.c", "join_order": c["order"],
                       "backend_schema": "pathmap-candidate-native-rematch-v1"},
            "demand": {"default": "bag", "supported": ["bag", "support"]},
            "certificate": os.path.relpath(os.path.join(a.out_dir, "certificate.metta"), ROOT),
        },
    }
    with open(os.path.join(a.out_dir, "manifest.json"), "w") as fh:
        json.dump(manifest, fh, indent=2)
        fh.write("\n")
    print(f"compiled {c['site']}: order={c['order']} meta_expanded_region_hash={manifest['identity']['meta_expanded_region_hash'][:16]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
