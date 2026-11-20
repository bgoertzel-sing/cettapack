;; fnspec-site: gv_once_filter
;; function: fn-gv-once-filter
;; origin: probe site (M1 KB, the probes' gv pattern), once-filter form (fnspec 2)
;; kb: m1
;; queries: gene
;; profile: extended
(= (fn-gv-once-filter $space $gene)
   (collapse (once (match $space (, (target $rs $gene) (expressed $gene $tissue)) (if (== $tissue tissue0) (empty) ($rs $tissue))))))
