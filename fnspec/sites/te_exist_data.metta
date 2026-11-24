;; fnspec-site: te_exist_data
;; function: fn-te-exist-data
;; origin: probe site (M1 KB, the probes' te pattern), exist-data form (fnspec 2)
;; kb: m1
;; queries: rs
;; profile: extended
(= (fn-te-exist-data $space $rs)
   (not (== (collapse (once (match $space (, (target $rs $gene) (expressed $gene $tissue)) true))) ())))
