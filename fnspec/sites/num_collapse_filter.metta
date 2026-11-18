;; fnspec-site: num_collapse_filter
;; function: num-above
;; origin: test-only site: numeric comparison in a collapse-filter (arithmetic edge values in the bag tests; not benchmarked)
;; kb: num
;; queries: num
(= (num-above $space $k $t)
   (collapse (match $space (val $k $x $y) (if (and (> $x $t) (not (<= $y $x))) ($k $x $y) (empty)))))
