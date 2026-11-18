;; fnspec-site: tf_stream_filter
;; function: fn-tf-stream-filter
;; origin: probe site tissue_fanout (packs/tissue_fanout/site.metta), stream-filter form (the probe's cpk-fn, docs/NEXT_STEP_PROBES.md)
;; kb: m1
;; queries: tissue
(= (fn-tf-stream-filter $space $tissue)
   (match $space (, (expressed $gene $tissue) (target $rs $gene)) (if (== $gene tissue0) (empty) ($rs $gene))))
