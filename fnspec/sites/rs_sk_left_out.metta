;; fnspec-site: rs_sk_left_out
;; function: sk-left-out-member
;; origin: PeTTaChainer 33cca343 pettachainer/metta/runtime/statistics/ledger.metta:146 (stream-cond: the inner match of the function at that line, which runs it per member hash under let/superpose and collapses it; a case on a bound value)
;; kb: sk_member
;; queries: sk_member
(= (sk-left-out-member $mhash $hash)
   (match &ctx_sk_instance_member ($mhash $hash $_stored $pos-seg $neg-seg $contribution)
            (case $contribution (
               ((InstanceEvidence (BaseRateEvidence $_pw 0.0) (BaseRateEvidence $_nw 0.0)) (empty))
               ($sample (instance-sample $pos-seg $neg-seg $sample))))))
