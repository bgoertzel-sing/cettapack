;; fnspec-site: gv_collapse_cond
;; function: fn-gv-collapse-cond
;; origin: probe site (M1 KB, the probes' gv pattern, gene_variants), collapse-cond (not a survey family: 0 occurrences; same compiler, kept as a test of the boolean leaf) (fnspec 3)
;; kb: m1
;; queries: gene
(= (fn-gv-collapse-cond $space $gene)
   (collapse (match $space (, (target $rs $gene) (expressed $gene $tissue)) (if (== $tissue tissue0) (empty) (== $tissue liver)))))
