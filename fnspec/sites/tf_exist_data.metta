;; fnspec-site: tf_exist_data
;; function: fn-tf-exist-data
;; origin: probe site (M1 KB, the probes' tf pattern), exist-data form (fnspec 2)
;; kb: m1
;; queries: tissue
;; profile: extended
(= (fn-tf-exist-data $space $tissue)
   (not (== (collapse (once (match $space (, (expressed $gene $tissue) (target $rs $gene)) true))) ())))
