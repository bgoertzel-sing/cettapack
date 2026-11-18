;; fnspec-site: gv_collapse_filter
;; function: fn-gv-collapse-filter
;; origin: probe site gene_variants (packs/gene_variants/site.metta), collapse-filter form
;; kb: m1
;; queries: gene
(= (fn-gv-collapse-filter $space $gene)
   (collapse (match $space (, (target $rs $gene) (expressed $gene $tissue)) (if (== $tissue tissue0) (empty) ($rs $tissue)))))
