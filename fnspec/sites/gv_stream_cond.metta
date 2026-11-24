;; fnspec-site: gv_stream_cond
;; function: fn-gv-stream-cond
;; origin: probe site (M1 KB, the probes' gv pattern, gene_variants), stream-cond (survey rank 12): a match whose template is a case on a bound value (fnspec 3)
;; kb: m1
;; queries: gene
(= (fn-gv-stream-cond $space $gene)
   (match $space (, (target $rs $gene) (expressed $gene $tissue)) (case $tissue ((tissue0 (empty)) (liver (hit $rs)) ($other ($rs $other))))))
