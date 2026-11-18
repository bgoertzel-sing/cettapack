;; fnspec-site: vi_collapse_filter
;; function: fn-vi-collapse-filter
;; origin: probe site variant_interactor (packs/variant_interactor/site.metta), collapse-filter form
;; kb: m1
;; queries: rs
(= (fn-vi-collapse-filter $space $rs)
   (collapse (match $space (, (target $rs $g1) (interacts $g2 $g1) (expressed $g2 $tissue)) (if (== $tissue tissue0) (empty) ($g2 $tissue)))))
