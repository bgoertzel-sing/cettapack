;; fnspec-site: vi_collapse_data
;; function: fn-vi-collapse-data
;; origin: probe site variant_interactor (packs/variant_interactor/site.metta), collapse-data form
;; kb: m1
;; queries: rs
(= (fn-vi-collapse-data $space $rs)
   (collapse (match $space (, (target $rs $g1) (interacts $g2 $g1) (expressed $g2 $tissue)) ($g2 $tissue))))
