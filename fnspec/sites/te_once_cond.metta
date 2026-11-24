;; fnspec-site: te_once_cond
;; function: fn-te-once-cond
;; origin: probe site (M1 KB, the probes' te pattern, target_expressed), once-cond (survey rank 9): collapse(once) of a match whose template is an if tree (fnspec 3)
;; kb: m1
;; queries: rs
;; profile: extended
(= (fn-te-once-cond $space $rs)
   (collapse (once (match $space (, (target $rs $gene) (expressed $gene $tissue)) (if (== $tissue tissue0) (empty) (if (and (== $tissue liver) (not (=alpha $gene $tissue))) (hit $gene) ($gene $tissue)))))))
