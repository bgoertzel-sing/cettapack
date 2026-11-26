;; S1 bag test t4: mid-run changes to the definition and to a builtin the template uses.
;; Expected with CETTA_S1_SITES=1: 2 calls specialised (baseline), 2 residual (second equation: two
;; equations of the function), 2 specialised (removed again, revalidated), 2 residual (=alpha shadowed
;; by an equation in &self), 2 specialised (unshadowed).
!(cpk-reg kb1 (Inheritance $x c1))
!(cpk-reg kb1 (Inheritance $x c2))
!(add-atom &self (= (forward-register-base-rate! $a $b $c $d $e $f $g) (cpk-extra)))
!(collapse (cpk-reg kb1 (Inheritance $x c3)))
!(collapse (cpk-reg kb1 (Inheritance $y c3)))
!(remove-atom &self (= (forward-register-base-rate! $a $b $c $d $e $f $g) (cpk-extra)))
!(cpk-reg kb1 (Inheritance $x c4))
!(cpk-reg kb1 (Inheritance $y c4))
!(add-atom &self (= (=alpha cpk-never-1 cpk-never-2) True))
!(cpk-reg kb1 (Inheritance $x c5))
!(cpk-reg kb1 (Inheritance $y c5))
!(remove-atom &self (= (=alpha cpk-never-1 cpk-never-2) True))
!(cpk-reg kb1 (Inheritance $x c6))
!(cpk-reg kb1 (Inheritance $y c6))
!(cpk-rows)
