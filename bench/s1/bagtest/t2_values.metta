;; S1 bag test t2: values outside what the site computes: a pattern holding a call CeTTa evaluates inside
;; =alpha (cpk-g has an equation), a string and numbers in patterns, a non-ground kb (input not ground).
;; Expected with CETTA_S1_SITES=1: the calls whose stored or given pattern holds (cpk-g ...) take the
;; residual (value guard), the non-ground kb call takes the residual (inputs), the rest specialise.
(= (cpk-g $x) (Done $x))
!(cpk-reg kb1 (Inheritance (cpk-g 1) c1))
!(cpk-reg kb1 (Inheritance (Done 1) c1))
!(cpk-reg kb1 (Inheritance "s" c1))
!(cpk-reg kb1 (Inheritance 1.5 c1))
!(cpk-reg kb1 (Inheritance 2 c1))
!(cpk-reg kb1 (Inheritance 2 c1))
!(cpk-reg $kb (Inheritance $x c3))
!(cpk-rows)
