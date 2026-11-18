;; fnspec-site: te_collapse_data
;; function: fn-te-collapse-data
;; origin: probe site target_expressed (packs/target_expressed/site.metta), collapse-data form
;; kb: m1
;; queries: rs
(= (fn-te-collapse-data $space $rs)
   (collapse (match $space (, (target $rs $gene) (expressed $gene $tissue)) ($gene $tissue))))
