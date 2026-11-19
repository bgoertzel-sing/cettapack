;; fnspec-site: num_stream_filter
;; function: num-eq
;; origin: test-only site: == on numbers and other kinds in a stream-filter (edge values; not benchmarked)
;; kb: num
;; queries: num
(= (num-eq $space $k $t)
   (match $space (val $k $x $y) (if (== $x $t) (empty) ($x $y))))
