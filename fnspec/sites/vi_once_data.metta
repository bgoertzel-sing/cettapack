;; fnspec-site: vi_once_data
;; function: fn-vi-once-data
;; origin: probe site (M1 KB, the probes' vi pattern), once-data form (fnspec 2)
;; kb: m1
;; queries: rs
;; profile: extended
(= (fn-vi-once-data $space $rs)
   (collapse (once (match $space (, (target $rs $g1) (interacts $g2 $g1) (expressed $g2 $tissue)) ($g2 $tissue)))))
