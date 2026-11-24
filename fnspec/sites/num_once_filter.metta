;; fnspec-site: num_once_filter
;; function: num-first-above
;; origin: test-only site: numeric comparison in a once-filter (edge values in the bag tests; not benchmarked)
;; kb: num
;; queries: num
;; profile: extended
(= (num-first-above $space $k $t)
   (collapse (once (match $space (val $k $x $y) (if (and (> $x $t) (not (<= $y $x))) ($k $x $y) (empty))))))
