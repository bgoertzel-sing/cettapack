;; S1 bag test t0: the first call of a run, before anything was ever added to &forward_base_rate_interest:
;; the space token is not bound yet, CeTTa runs that match as a plain-symbol (Prolog-predicate) query before
;; the site hook is reached, so the site is not offered it; from the second call on it is.  Here the space is
;; first used by an add and remove of an unrelated row, so every one of the 3 calls is offered and recognised.
!(add-atom &forward_base_rate_interest (cpk-unrelated-row))
!(remove-atom &forward_base_rate_interest (cpk-unrelated-row))
!(cpk-reg kb1 (Inheritance $x c1))
!(cpk-reg kb1 (Inheritance $y c1))
!(cpk-reg kb1 (Inheritance $x c2))
!(cpk-rows)
