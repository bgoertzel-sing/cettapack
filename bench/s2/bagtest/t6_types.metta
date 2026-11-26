;; S2 bag test t6: calls whose operands do not have the declared types (CeTTa's answer, whatever it is, is the
;; reference), calls inside the chainer's own control (let*, case, collapse) and a call whose expected result is
;; already bound (the answer is unified with it).
!(tv-confidence 7)
!(tv-confidence "s")
!(frontier-pool-potential 5)
!(premises-expected-confidence 3)
!(and-bundle-premises notabundle)
!(expr-arity foo)
!(proof-output-key-by-id foo)
!(proof-output-key-by-id 1.5)
!(let* (($x (tv-confidence (STV 0.1 0.6))) ($y (frontier-pool-potential ((pool-part k (STV 1.0 0.8) 2 () ev mtv))))) ($x $y))
!(case (tv-confidence (STV 0.1 0.6)) ((0.6 yes) ($_ no)))
!(case (tv-confidence (STV 0.1 0.6)) ((0.5 yes) ($_ no)))
!(collapse (let 0.6 (tv-confidence (STV 0.1 0.6)) matched))
!(collapse (let 0.5 (tv-confidence (STV 0.1 0.6)) matched))
!(collapse (let (and-bundle g $p) (make-and-bundle g (x y)) $p))
