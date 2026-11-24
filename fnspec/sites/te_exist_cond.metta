;; fnspec-site: te_exist_cond
;; function: fn-te-exist-cond
;; origin: probe site (M1 KB, the probes' te pattern, target_expressed), exist-cond (survey rank 11): existence test of a match whose template is a nested if (fnspec 3)
;; kb: m1
;; queries: rs
;; profile: extended
(= (fn-te-exist-cond $space $rs)
   (not (== (collapse (once (match $space (, (target $rs $gene) (expressed $gene $tissue)) (if (== $tissue tissue0) (empty) (if (== $tissue liver) (empty) true))))) ())))
