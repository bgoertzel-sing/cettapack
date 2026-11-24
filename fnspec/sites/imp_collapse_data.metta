;; fnspec-site: imp_collapse_data
;; function: imp-te-collapse-data
;; origin: test-only site (item 2): te_collapse_data's function (fnspec 1 shape) defined in a module that &self imports (import!); not benchmarked
;; kb: m1
;; queries: rs
;; load: import
(= (imp-te-collapse-data $space $rs)
   (collapse (match $space (, (target $rs $gene) (expressed $gene $tissue)) ($gene $tissue))))
