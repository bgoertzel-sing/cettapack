;; fnspec-site: gv_stream_data
;; function: fn-gv-stream-data
;; origin: probe site gene_variants (packs/gene_variants/site.metta), stream-data form
;; kb: m1
;; queries: gene
(= (fn-gv-stream-data $space $gene)
   (match $space (, (target $rs $gene) (expressed $gene $tissue)) ($rs $tissue)))
