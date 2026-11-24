;; fnspec-site: gv_exist_cond
;; function: fn-gv-exist-cond
;; origin: probe site (M1 KB, the probes' gv pattern, gene_variants), exist-cond (survey rank 11): existence test of a match whose template is a nested if (fnspec 3)
;; kb: m1
;; queries: gene
;; profile: extended
(= (fn-gv-exist-cond $space $gene)
   (not (== (collapse (once (match $space (, (target $rs $gene) (expressed $gene $tissue)) (if (== $tissue tissue0) (empty) (if (== $tissue liver) (empty) true))))) ())))
