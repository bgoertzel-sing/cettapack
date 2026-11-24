;; fnspec-site: vi_exist_cond
;; function: fn-vi-exist-cond
;; origin: probe site (M1 KB, the probes' vi pattern, variant_interactor), exist-cond (survey rank 11): existence test of a match whose template is a nested if (fnspec 3)
;; kb: m1
;; queries: rs
;; profile: extended
(= (fn-vi-exist-cond $space $rs)
   (not (== (collapse (once (match $space (, (target $rs $g1) (interacts $g2 $g1) (expressed $g2 $tissue)) (if (== $tissue tissue0) (empty) (if (== $tissue liver) (empty) true))))) ())))
