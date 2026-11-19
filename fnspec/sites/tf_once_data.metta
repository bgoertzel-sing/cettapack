;; fnspec-site: tf_once_data
;; function: fn-tf-once-data
;; origin: probe site (M1 KB, the probes' tf pattern), once-data form (fnspec 2)
;; kb: m1
;; queries: tissue
;; profile: extended
(= (fn-tf-once-data $space $tissue)
   (collapse (once (match $space (, (expressed $gene $tissue) (target $rs $gene)) ($rs $gene)))))
