;; fnspec-site: te_collapse_filter
;; function: fn-te-collapse-filter
;; origin: probe site target_expressed (packs/target_expressed/site.metta), collapse-filter form
;; kb: m1
;; queries: rs
(= (fn-te-collapse-filter $space $rs)
   (collapse (match $space (, (target $rs $gene) (expressed $gene $tissue)) (if (== $tissue tissue0) (empty) ($gene $tissue)))))
