;; fnspec-site: tf_stream_data
;; function: fn-tf-stream-data
;; origin: probe site tissue_fanout (packs/tissue_fanout/site.metta), stream-data form
;; kb: m1
;; queries: tissue
(= (fn-tf-stream-data $space $tissue)
   (match $space (, (expressed $gene $tissue) (target $rs $gene)) ($rs $gene)))
