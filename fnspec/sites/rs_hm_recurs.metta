;; fnspec-site: rs_hm_recurs
;; function: hm-recurs-seen
;; origin: PeTTaChainer 33cca343 pettachainer/metta/runtime/hypothesis_mining.metta:83 (once-cond: the (collapse (once ...)) the function at that line takes as its case scrutinee; the case adds a row when it is (); a boolean template leaf; profile extended)
;; kb: hm_seen
;; queries: hm_seen
;; profile: extended
(= (hm-recurs-seen $kb $hash $rule $instance $context)
   (collapse (once (match &hm_seen ($hash $kb $stored $first $seen)
            (if (=alpha $stored $rule) (not (or (== $first $instance) (== $seen $context))) (empty))))))
