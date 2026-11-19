;; fnspec-site: tf_collapse_filter
;; function: fn-tf-collapse-filter
;; origin: probe site tissue_fanout (packs/tissue_fanout/site.metta), collapse-filter form
;; kb: m1
;; queries: tissue
(= (fn-tf-collapse-filter $space $tissue)
   (collapse (match $space (, (expressed $gene $tissue) (target $rs $gene)) (if (== $gene tissue0) (empty) ($rs $gene)))))
