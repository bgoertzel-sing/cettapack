;; fnspec-site: gv_once_cond
;; function: fn-gv-once-cond
;; origin: probe site (M1 KB, the probes' gv pattern, gene_variants), once-cond (survey rank 9): collapse(once) of a match whose template is an if tree (fnspec 3)
;; kb: m1
;; queries: gene
;; profile: extended
(= (fn-gv-once-cond $space $gene)
   (collapse (once (match $space (, (target $rs $gene) (expressed $gene $tissue)) (if (== $tissue tissue0) (empty) (if (and (== $tissue liver) (not (=alpha $rs $tissue))) (hit $rs) ($rs $tissue)))))))
