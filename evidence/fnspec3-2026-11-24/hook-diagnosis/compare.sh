#!/bin/bash
cd /home/openclaw/research-agent/projects/cettapack/wt/fnspec-3
pkill -f 'f3diag/cpk-diag3 bench' 2>/dev/null
for s in gv_collapse_cond rs_sk_needed te_once_filter tf_collapse_data rs_given_rule; do
  args=$(python3 bench/fnspec_inputs.py --site $s --build build); line="$s:"
  for k in 1 2 3; do for b in fn-spec:0 fn-rows:0 fn-rows:1; do
    m=${b%%:*}; q=${b##*:}
    if [ $q = 1 ]; then export CPK_ROWS_QUERY=1; else unset CPK_ROWS_QUERY; fi
    v=$(nice -n 19 /tmp/f3diag/cpk-diag3 bench --site $s $args --mode $m --repeat 10 2>&1 | python3 -c "
import sys,json
for l in sys.stdin:
    if l.startswith('{'):
        d=json.loads(l); print('%.0f/%.0f/%d/%d' % (d['per_query_us_median'], d['inner_ns']/1000/max(d['fn_calls'],1), d['fn_spec_calls'], d['answers_per_batch']))
")
    line="$line $m$q=$v"
  done; done
  echo "$line" >> /tmp/f3_cmp3.out
done
echo DONE >> /tmp/f3_cmp3.out
