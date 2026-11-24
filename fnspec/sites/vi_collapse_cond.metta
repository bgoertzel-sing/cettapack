;; fnspec-site: vi_collapse_cond
;; function: fn-vi-collapse-cond
;; origin: probe site (M1 KB, the probes' vi pattern, variant_interactor), collapse-cond (not a survey family: 0 occurrences; same compiler, kept as a test of the boolean leaf) (fnspec 3)
;; kb: m1
;; queries: rs
(= (fn-vi-collapse-cond $space $rs)
   (collapse (match $space (, (target $rs $g1) (interacts $g2 $g1) (expressed $g2 $tissue)) (if (== $tissue tissue0) (empty) (== $tissue liver)))))
