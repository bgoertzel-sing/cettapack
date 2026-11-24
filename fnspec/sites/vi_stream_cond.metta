;; fnspec-site: vi_stream_cond
;; function: fn-vi-stream-cond
;; origin: probe site (M1 KB, the probes' vi pattern, variant_interactor), stream-cond (survey rank 12): a match whose template is a case on a bound value (fnspec 3)
;; kb: m1
;; queries: rs
(= (fn-vi-stream-cond $space $rs)
   (match $space (, (target $rs $g1) (interacts $g2 $g1) (expressed $g2 $tissue)) (case $tissue ((tissue0 (empty)) (liver (hit $g2)) ($other ($g2 $other))))))
