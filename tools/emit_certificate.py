#!/usr/bin/env python3
"""Emit the per-pack certificate skeleton (A.8 adaptation) from a test run.

Claim level bag-tested-adequate: on every tested input where the guard said
VALID, the kernel's answer bag equaled generic CeTTa's (multiplicities
included), and every guard failure fell back to a bag equal to generic
CeTTa's. Nothing is proved; the proof obligations are listed as open.
"""
import hashlib
import json
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def sha(path):
    with open(path, "rb") as fh:
        return hashlib.sha256(fh.read()).hexdigest()


def main(pack_dir, test_log, kb_path):
    manifest_path = os.path.join(pack_dir, "manifest.json")
    manifest = json.load(open(manifest_path))
    results = [json.loads(l) for l in open(test_log) if l.strip()]
    passed = [r for r in results if r["ok"]]
    failed = [r for r in results if not r["ok"]]
    if failed:
        print(f"refusing to certify: {len(failed)} test item(s) failed", file=sys.stderr)
        return 1
    head = subprocess.check_output(["git", "-C", ROOT, "rev-parse", "HEAD"], text=True).strip()
    cert = {
        "certificate": "cettapack-cert-skeleton/0.1",
        "pack_id": manifest["pack_id"],
        "site": manifest["site"]["name"],
        "manifest_sha256": sha(manifest_path),
        "plan_sha256": manifest["kernel"]["plan_sha256"],
        "structure": {k: manifest["structure"][k] for k in ("region_hash", "pattern_hash", "template_hash")},
        "profile_digest": manifest["profile"]["digest"],
        "claim_level": "bag-tested-adequate",
        "claim": ("For the tested inputs, whenever the guard returned VALID the kernel's answer bag "
                  "equaled generic CeTTa's (multiplicities included), and whenever it did not, the "
                  "residual fallback's bag equaled generic CeTTa's."),
        "conditions": [
            "profile, langdef, structural and schema digests equal the manifest's",
            "the matched space is the instantiated one, at the indexed revision and length",
            "every record that could unify with a factor is a flat all-symbol record of the compiled schema",
            "seeded inputs are ground symbols",
            "PathMap narrowing is enabled and its index self-check passed",
        ],
        "evidence": {
            "tests": [{"item": r["item"], "name": r["name"]} for r in passed],
            "test_log_sha256": sha(test_log),
            "kb_sha256": sha(kb_path) if os.path.exists(kb_path) else None,
            "kb_generator": "bench/gen_kb.py --seed 20261106 (defaults)",
            "cettapack_commit": head,
            "cetta_commit": manifest["profile"]["components"]["cetta"],
        },
        "proof": None,
        "open_obligations": [
            "adequacy: VALID guard => kernel bag = generic bag for all spaces (unproved; tested only)",
            "narrowing completeness: every record that can match a factor is reachable from its key prefix (argued in docs/DESIGN.md, tested)",
            "rematch soundness is CeTTa's (space_match_backend_match_atom_seeded); not re-verified",
            "Lean formalization: not part of M1",
        ],
    }
    out = os.path.join(pack_dir, "certificate.json")
    with open(out, "w") as fh:
        json.dump(cert, fh, indent=2)
        fh.write("\n")
    print(f"wrote {os.path.relpath(out, ROOT)}: {len(passed)} items, claim {cert['claim_level']}")
    return 0


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:4]))
