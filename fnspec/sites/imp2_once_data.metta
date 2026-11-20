;; fnspec-site: imp2_once_data
;; function: imp2-te-once-data
;; origin: test-only site (item 2): te_once_data's function defined in a module B that a module A imports, A imported by &self (two levels of import!); not benchmarked
;; kb: m1
;; queries: rs
;; profile: extended
;; load: import2
(= (imp2-te-once-data $space $rs)
   (collapse (once (match $space (, (target $rs $gene) (expressed $gene $tissue)) ($gene $tissue)))))
