;; fnspec-site: tf_stream_cond
;; function: fn-tf-stream-cond
;; origin: probe site (M1 KB, the probes' tf pattern, tissue_fanout), stream-cond (survey rank 12): a match whose template is a case on a bound value (fnspec 3)
;; kb: m1
;; queries: tissue
(= (fn-tf-stream-cond $space $tissue)
   (match $space (, (expressed $gene $tissue) (target $rs $gene)) (case $gene ((g1 (empty)) (g2 (hit $rs)) ($other ($rs $other))))))
