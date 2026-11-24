;; fnspec-site: num_exist_filter
;; function: num-none-above
;; origin: test-only site: numeric comparison in an existence test without once, (== W ()) polarity (edge values in the bag tests; not benchmarked)
;; kb: num
;; queries: num
(= (num-none-above $space $k $t)
   (== (collapse (match $space (val $k $x $y) (if (and (> $x $t) (not (<= $y $x))) ($k $x $y) (empty)))) ()))
