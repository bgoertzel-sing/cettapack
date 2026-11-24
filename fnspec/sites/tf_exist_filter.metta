;; fnspec-site: tf_exist_filter
;; function: fn-tf-exist-filter
;; origin: probe site (M1 KB, the probes' tf pattern), exist-filter form (fnspec 2)
;; kb: m1
;; queries: tissue
;; profile: extended
(= (fn-tf-exist-filter $space $tissue)
   (not (== (collapse (once (match $space (, (expressed $gene $tissue) (target $rs $gene)) (if (== $gene tissue0) (empty) true)))) ())))
