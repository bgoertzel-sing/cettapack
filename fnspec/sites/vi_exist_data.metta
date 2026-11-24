;; fnspec-site: vi_exist_data
;; function: fn-vi-exist-data
;; origin: probe site (M1 KB, the probes' vi pattern), exist-data form (fnspec 2)
;; kb: m1
;; queries: rs
;; profile: extended
(= (fn-vi-exist-data $space $rs)
   (not (== (collapse (once (match $space (, (target $rs $g1) (interacts $g2 $g1) (expressed $g2 $tissue)) true))) ())))
