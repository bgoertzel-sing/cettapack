;; fnspec-site: te_once_data
;; function: fn-te-once-data
;; origin: probe site (M1 KB, the probes' te pattern), once-data form (fnspec 2)
;; kb: m1
;; queries: rs
;; profile: extended
(= (fn-te-once-data $space $rs)
   (collapse (once (match $space (, (target $rs $gene) (expressed $gene $tissue)) ($gene $tissue)))))
