;; fnspec-site: tf_once_filter
;; function: fn-tf-once-filter
;; origin: probe site (M1 KB, the probes' tf pattern), once-filter form (fnspec 2)
;; kb: m1
;; queries: tissue
;; profile: extended
(= (fn-tf-once-filter $space $tissue)
   (collapse (once (match $space (, (expressed $gene $tissue) (target $rs $gene)) (if (== $gene tissue0) (empty) ($rs $gene))))))
