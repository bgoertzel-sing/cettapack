;; S1 bag test t3: two stored rows that both pass for one call (two ids): the site's order guard sends the
;; call to the residual (once then takes the machine's first row).  Then one row passes again.
!(cpk-reg kb1 (Inheritance $x c1))
!(add-atom &forward_base_rate_interest (forward-base-rate-interest-row (fwd_trigger kb1 Inheritance) 999 kb1 (Inheritance $q c1) extract-tv (BaseRateEvidence 0.0 0.0) BaseRateAcc base-rate result-tv))
!(cpk-reg kb1 (Inheritance $y c1))
!(cpk-reg kb1 (Inheritance $y c2))
!(cpk-rows)
