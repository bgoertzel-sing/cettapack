;; fnspec-site: rs_sk_row_present
;; function: sk-row-present?
;; origin: PeTTaChainer 33cca343 pettachainer/metta/runtime/backward/skeleton.metta:643 (exist-cond, the whole function: an existence test over a nested if; profile extended)
;; kb: sk_row
;; queries: sk_row
;; profile: extended
(= (sk-row-present? $khash $key $rhash $ident)
   (not (== (collapse (once (match &sk_row ($khash $stored $rhash (sk-row $stored-ident $_c $_p $_seq))
               (if (=alpha $stored $key) (if (=alpha $stored-ident $ident) true (empty)) (empty)))))
            ())))
