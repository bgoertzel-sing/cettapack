;; fnspec-site: gv_stream_filter
;; function: fn-gv-stream-filter
;; origin: probe site gene_variants (packs/gene_variants/site.metta), stream-filter form (the probe's cpk-fn, docs/NEXT_STEP_PROBES.md)
;; kb: m1
;; queries: gene
(= (fn-gv-stream-filter $space $gene)
   (match $space (, (target $rs $gene) (expressed $gene $tissue)) (if (== $tissue tissue0) (empty) ($rs $tissue))))
