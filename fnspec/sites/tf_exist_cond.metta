;; fnspec-site: tf_exist_cond
;; function: fn-tf-exist-cond
;; origin: probe site (M1 KB, the probes' tf pattern, tissue_fanout), exist-cond (survey rank 11): existence test of a match whose template is a nested if (fnspec 3)
;; kb: m1
;; queries: tissue
;; profile: extended
(= (fn-tf-exist-cond $space $tissue)
   (not (== (collapse (once (match $space (, (expressed $gene $tissue) (target $rs $gene)) (if (== $gene g1) (empty) (if (== $gene g2) (empty) true))))) ())))
