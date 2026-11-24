;; fnspec-site: rs_sk_summary_memo
;; function: sk-summary-memo
;; origin: PeTTaChainer 33cca343 pettachainer/metta/runtime/evaluator/core.metta:258 (once-cond: the (collapse (once ...)) the function at that line takes as its case scrutinee, a four-level if tree; profile extended)
;; kb: sk_summary
;; queries: sk_summary
;; profile: extended
(= (sk-summary-memo $hash $key $rhash $given $exclude $filter)
   (collapse (once (match &ctx_sk_summary_reference
                  ($hash $memo-key $rhash $memo-pins $memo-cuts $memo-filter $memo-value)
                  (if (=alpha $memo-key $key)
                     (if (=alpha $memo-pins $given)
                        (if (== $memo-cuts $exclude) (if (=alpha $memo-filter $filter) $memo-value (empty)) (empty))
                        (empty)) (empty))))))
