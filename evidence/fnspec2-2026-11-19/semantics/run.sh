#!/usr/bin/env bash
# usage: evidence/fnspec2-2026-11-19/semantics/run.sh  (from the repo root, after scripts/build_cetta.sh)
set -euo pipefail
C=build/cetta-src/cetta
F=evidence/fnspec2-2026-11-19/semantics/once_profiles.metta
for p in he he-compat extended he-prime; do echo "== --lang he --profile $p"; $C --lang he --profile $p $F 2>&1 | tail -6 | tr '\n' ' '; echo; done
echo "== PeTTa (default language)"; $C $F 2>&1 | tail -6 | tr '\n' ' '; echo
