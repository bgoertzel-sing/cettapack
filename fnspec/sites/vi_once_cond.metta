;; fnspec-site: vi_once_cond
;; function: fn-vi-once-cond
;; origin: probe site (M1 KB, the probes' vi pattern, variant_interactor), once-cond (survey rank 9): collapse(once) of a match whose template is an if tree (fnspec 3)
;; kb: m1
;; queries: rs
;; profile: extended
(= (fn-vi-once-cond $space $rs)
   (collapse (once (match $space (, (target $rs $g1) (interacts $g2 $g1) (expressed $g2 $tissue)) (if (== $tissue tissue0) (empty) (if (and (== $tissue liver) (not (=alpha $g2 $tissue))) (hit $g2) ($g2 $tissue)))))))
