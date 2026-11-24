;; fnspec-site: tf_collapse_cond
;; function: fn-tf-collapse-cond
;; origin: probe site (M1 KB, the probes' tf pattern, tissue_fanout), collapse-cond (not a survey family: 0 occurrences; same compiler, kept as a test of the boolean leaf) (fnspec 3)
;; kb: m1
;; queries: tissue
(= (fn-tf-collapse-cond $space $tissue)
   (collapse (match $space (, (expressed $gene $tissue) (target $rs $gene)) (if (== $gene g1) (empty) (== $gene g2)))))
