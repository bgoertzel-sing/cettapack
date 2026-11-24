;; fnspec-site: te_collapse_cond
;; function: fn-te-collapse-cond
;; origin: probe site (M1 KB, the probes' te pattern, target_expressed), collapse-cond (not a survey family: 0 occurrences; same compiler, kept as a test of the boolean leaf) (fnspec 3)
;; kb: m1
;; queries: rs
(= (fn-te-collapse-cond $space $rs)
   (collapse (match $space (, (target $rs $gene) (expressed $gene $tissue)) (if (== $tissue tissue0) (empty) (== $tissue liver)))))
