;; fnspec-site: imp_exist_filter
;; function: imp-te-exist-filter
;; origin: test-only site (item 2): te_exist_filter's function defined in a module that &self imports (import!), not in &self; not benchmarked
;; kb: m1
;; queries: rs
;; profile: extended
;; load: import
(= (imp-te-exist-filter $space $rs)
   (not (== (collapse (once (match $space (, (target $rs $gene) (expressed $gene $tissue)) (if (== $tissue tissue0) (empty) true)))) ())))
