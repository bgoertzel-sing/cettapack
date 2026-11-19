;; fnspec-site: vi_exist_filter
;; function: fn-vi-exist-filter
;; origin: probe site (M1 KB, the probes' vi pattern), exist-filter form (fnspec 2)
;; kb: m1
;; queries: rs
;; profile: extended
(= (fn-vi-exist-filter $space $rs)
   (not (== (collapse (once (match $space (, (target $rs $g1) (interacts $g2 $g1) (expressed $g2 $tissue)) (if (== $tissue tissue0) (empty) true)))) ())))
