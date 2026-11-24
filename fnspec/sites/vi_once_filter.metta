;; fnspec-site: vi_once_filter
;; function: fn-vi-once-filter
;; origin: probe site (M1 KB, the probes' vi pattern), once-filter form (fnspec 2)
;; kb: m1
;; queries: rs
;; profile: extended
(= (fn-vi-once-filter $space $rs)
   (collapse (once (match $space (, (target $rs $g1) (interacts $g2 $g1) (expressed $g2 $tissue)) (if (== $tissue tissue0) (empty) ($g2 $tissue))))))
