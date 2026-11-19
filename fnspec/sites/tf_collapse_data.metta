;; fnspec-site: tf_collapse_data
;; function: fn-tf-collapse-data
;; origin: probe site tissue_fanout (packs/tissue_fanout/site.metta), collapse-data form
;; kb: m1
;; queries: tissue
(= (fn-tf-collapse-data $space $tissue)
   (collapse (match $space (, (expressed $gene $tissue) (target $rs $gene)) ($rs $gene))))
