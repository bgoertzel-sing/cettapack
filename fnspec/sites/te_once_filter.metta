;; fnspec-site: te_once_filter
;; function: fn-te-once-filter
;; origin: probe site (M1 KB, the probes' te pattern), once-filter form (fnspec 2)
;; kb: m1
;; queries: rs
;; profile: extended
(= (fn-te-once-filter $space $rs)
   (collapse (once (match $space (, (target $rs $gene) (expressed $gene $tissue)) (if (== $tissue tissue0) (empty) ($gene $tissue))))))
