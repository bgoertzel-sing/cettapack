;; fnspec-site: te_stream_data
;; function: fn-te-stream-data
;; origin: probe site target_expressed (packs/target_expressed/site.metta), stream-data form
;; kb: m1
;; queries: rs
(= (fn-te-stream-data $space $rs)
   (match $space (, (target $rs $gene) (expressed $gene $tissue)) ($gene $tissue)))
