;; fnspec-site: te_exist_filter
;; function: fn-te-exist-filter
;; origin: probe site (M1 KB, the probes' te pattern), exist-filter form (fnspec 2)
;; kb: m1
;; queries: rs
;; profile: extended
(= (fn-te-exist-filter $space $rs)
   (not (== (collapse (once (match $space (, (target $rs $gene) (expressed $gene $tissue)) (if (== $tissue tissue0) (empty) true)))) ())))
