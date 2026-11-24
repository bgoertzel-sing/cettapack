;; fnspec-site: tf_once_cond
;; function: fn-tf-once-cond
;; origin: probe site (M1 KB, the probes' tf pattern, tissue_fanout), once-cond (survey rank 9): collapse(once) of a match whose template is an if tree (fnspec 3)
;; kb: m1
;; queries: tissue
;; profile: extended
(= (fn-tf-once-cond $space $tissue)
   (collapse (once (match $space (, (expressed $gene $tissue) (target $rs $gene)) (if (== $gene g1) (empty) (if (and (== $gene g2) (not (=alpha $rs $gene))) (hit $rs) ($rs $gene)))))))
