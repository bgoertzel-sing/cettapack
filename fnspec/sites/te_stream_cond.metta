;; fnspec-site: te_stream_cond
;; function: fn-te-stream-cond
;; origin: probe site (M1 KB, the probes' te pattern, target_expressed), stream-cond (survey rank 12): a match whose template is a case on a bound value (fnspec 3)
;; kb: m1
;; queries: rs
(= (fn-te-stream-cond $space $rs)
   (match $space (, (target $rs $gene) (expressed $gene $tissue)) (case $tissue ((tissue0 (empty)) (liver (hit $gene)) ($other ($gene $other))))))
