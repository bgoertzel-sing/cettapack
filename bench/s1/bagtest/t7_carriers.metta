;; S1 bag test t7: patterns the chainer builds as PeTTa list carriers ((cons H T) evaluated), as in the real
;; runs: given patterns and stored rows in carrier form, alpha-variants across carrier and flat forms.
;; Expected with CETTA_S1_SITES=1 (first call not offered): 5 recognised, 5 specialised.
;; Not here: a carrier with a variable tail, (cpk-reg kb1 (cons Inheritance $t)): CeTTa itself stops on it with
;; "observation incomplete: host-failure" in every mode (pilot, evidence/s1-2026-11-24/bagtest-t7-pilot); the
;; site declines that call (residual: the pattern holds a list carrier the query cannot read).
(= (cpk-mk $c) (cons Inheritance (cons $x (cons $c ()))))
(= (cpk-mk2 $a $c) (cons Inheritance (cons $a (cons $c ()))))
!(cpk-reg kb1 (cpk-mk c1))
!(cpk-reg kb1 (Inheritance $y c1))
!(cpk-reg kb1 (cpk-mk c1))
!(cpk-reg kb1 (cpk-mk2 a c2))
!(cpk-reg kb1 (Inheritance a c2))
!(cpk-reg kb1 (cpk-mk c3))
!(cpk-rows)
