;; fnspec-site: gv_collapse_data
;; function: fn-gv-collapse-data
;; origin: probe site gene_variants (packs/gene_variants/site.metta), collapse-data form
;; kb: m1
;; queries: gene
(= (fn-gv-collapse-data $space $gene)
   (collapse (match $space (, (target $rs $gene) (expressed $gene $tissue)) ($rs $tissue))))
