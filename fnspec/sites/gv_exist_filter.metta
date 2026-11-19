;; fnspec-site: gv_exist_filter
;; function: fn-gv-exist-filter
;; origin: probe site (M1 KB, the probes' gv pattern), exist-filter form (fnspec 2)
;; kb: m1
;; queries: gene
;; profile: extended
(= (fn-gv-exist-filter $space $gene)
   (not (== (collapse (once (match $space (, (target $rs $gene) (expressed $gene $tissue)) (if (== $tissue tissue0) (empty) true)))) ())))
