;; S1 bag test t5: the function replaced mid-run by a different equation that still holds the same match
;; (its body wrapped in a let), then restored.  Expected with CETTA_S1_SITES=1: 1 specialised, 2 residual
;; (the equation differs from the compiled one), 1 specialised after the original is back.
!(cpk-reg kb1 (Inheritance $x c1))
!(match &self (= (forward-register-base-rate! $a $b $c $d $e $f $g) $body) (let $_ (remove-atom &self (= (forward-register-base-rate! $a $b $c $d $e $f $g) $body)) (add-atom &self (cpk-saved (= (forward-register-base-rate! $a $b $c $d $e $f $g) $body)))))
!(match &self (cpk-saved (= (forward-register-base-rate! $a $b $c $d $e $f $g) $body)) (add-atom &self (= (forward-register-base-rate! $a $b $c $d $e $f $g) (let $cpk-unused 0 $body))))
!(cpk-reg kb1 (Inheritance $x c2))
!(cpk-reg kb1 (Inheritance $y c2))
!(match &self (= (forward-register-base-rate! $a $b $c $d $e $f $g) (let $cpk-unused 0 $body)) (remove-atom &self (= (forward-register-base-rate! $a $b $c $d $e $f $g) (let $cpk-unused 0 $body))))
!(match &self (cpk-saved $eq) (add-atom &self $eq))
!(cpk-reg kb1 (Inheritance $y c3))
!(cpk-rows)
