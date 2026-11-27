#!/usr/bin/env bash
# cetta-internals P: regenerate every table of docs/P_PROFILE.md from the committed evidence.
#   bash bench/p/recompute_p.sh      # writes evidence/p-2026-11-27/*.md|json (sampler tables need build/p/sym or
#                                    # the committed sampler/*.json)
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
E=evidence/p-2026-11-27
python3 bench/p/recheck_ev.py $E/matrix/ev $E/matrix/evproto > /dev/null
python3 bench/p/evprof_report.py $E/matrix/ev --off $E/matrix/off --json $E/evprof.json --md $E/evprof.md --top 40
python3 bench/p/evprof_report.py $E/matrix/ev --off $E/matrix/off --residual --json $E/evprof-residual.json \
  --md $E/evprof-residual.md --top 40
python3 bench/p/evprof_report.py $E/runs-contended/ev --json $E/evprof-contended.json --md $E/evprof-contended.md
python3 bench/p/sampler_table.py $E/sampler $E/evprof.json --md $E/sampler.md
python3 bench/p/stats_report.py $E/runs-contended/stats --json $E/stats.json --md $E/stats.md
python3 bench/p/ablation_report.py $E/matrix --base off --json $E/matrix.json --md $E/matrix.md
python3 bench/p/ablation_report.py $E/matrix2 --base off --json $E/matrix2.json --md $E/matrix2.md
python3 bench/p/proto_sites.py $E/matrix/ev $E/matrix/evproto --json $E/proto_sites.json --md $E/proto_sites.md
python3 bench/p/proto_fraction.py $E/proto_sites.json $E/proto_fraction.json
python3 bench/p/estimates.py $E/evprof.json --matrix $E/matrix.json --json $E/estimates.json --md $E/estimates.md
python3 bench/p/estimates.py $E/evprof-residual.json --matrix $E/matrix.json --json $E/estimates-residual.json \
  --md $E/estimates-residual.md
echo recomputed
