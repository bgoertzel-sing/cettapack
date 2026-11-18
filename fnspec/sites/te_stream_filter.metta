;; fnspec-site: te_stream_filter
;; function: fn-te-stream-filter
;; origin: probe site target_expressed (packs/target_expressed/site.metta), stream-filter form (the probe's cpk-fn, docs/NEXT_STEP_PROBES.md)
;; kb: m1
;; queries: rs
(= (fn-te-stream-filter $space $rs)
   (match $space (, (target $rs $gene) (expressed $gene $tissue)) (if (== $tissue tissue0) (empty) ($gene $tissue))))
