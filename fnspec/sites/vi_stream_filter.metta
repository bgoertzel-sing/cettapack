;; fnspec-site: vi_stream_filter
;; function: fn-vi-stream-filter
;; origin: probe site variant_interactor (packs/variant_interactor/site.metta), stream-filter form (the probe's cpk-fn, docs/NEXT_STEP_PROBES.md)
;; kb: m1
;; queries: rs
(= (fn-vi-stream-filter $space $rs)
   (match $space (, (target $rs $g1) (interacts $g2 $g1) (expressed $g2 $tissue)) (if (== $tissue tissue0) (empty) ($g2 $tissue))))
