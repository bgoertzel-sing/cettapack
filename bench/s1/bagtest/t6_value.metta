;; S1 bag test t6: a stored pattern that becomes evaluable: the row is stored while cpk-h has no equation
;; (so (cpk-h 1) is data), then cpk-h gets one; PeTTa's =alpha would now evaluate (cpk-h 1) in that row, which
;; the site does not compute: the value guard sends the call to the residual.  Then the row is removed and the
;; calls specialise again.  Expected with CETTA_S1_SITES=1 (first call not offered): 3 recognised,
;; 1 residual (value), 2 specialised.
!(cpk-reg kb1 (Inheritance (cpk-h 1) c1))
!(add-atom &self (= (cpk-h $x) (Done $x)))
!(cpk-reg kb1 (Inheritance $y c2))
!(match &forward_base_rate_interest (forward-base-rate-interest-row $t $id $k (Inheritance (cpk-h 1) c1) $e $i $f $fi $m) (remove-atom &forward_base_rate_interest (forward-base-rate-interest-row $t $id $k (Inheritance (cpk-h 1) c1) $e $i $f $fi $m)))
!(cpk-reg kb1 (Inheritance $y c3))
!(cpk-reg kb1 (Inheritance $z c3))
!(cpk-rows)
