;; fnspec-site: gv_once_data
;; function: fn-gv-once-data
;; origin: probe site (M1 KB, the probes' gv pattern), once-data form (fnspec 2)
;; kb: m1
;; queries: gene
;; profile: extended
(= (fn-gv-once-data $space $gene)
   (collapse (once (match $space (, (target $rs $gene) (expressed $gene $tissue)) ($rs $tissue)))))
