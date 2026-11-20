;; fnspec-site: gv_exist_data
;; function: fn-gv-exist-data
;; origin: probe site (M1 KB, the probes' gv pattern), exist-data form (fnspec 2)
;; kb: m1
;; queries: gene
;; profile: extended
(= (fn-gv-exist-data $space $gene)
   (not (== (collapse (once (match $space (, (target $rs $gene) (expressed $gene $tissue)) true))) ())))
