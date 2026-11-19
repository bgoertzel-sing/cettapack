;; fnspec-site: vi_stream_data
;; function: fn-vi-stream-data
;; origin: probe site variant_interactor (packs/variant_interactor/site.metta), stream-data form
;; kb: m1
;; queries: rs
(= (fn-vi-stream-data $space $rs)
   (match $space (, (target $rs $g1) (interacts $g2 $g1) (expressed $g2 $tissue)) ($g2 $tissue)))
