;; fnspec-site: rs_sk_needed
;; function: sk-needed?
;; origin: PeTTaChainer 33cca343 pettachainer/metta/runtime/evaluator/versions.metta:1242 (exist-filter with once; PeTTa type declaration dropped, run in CeTTa's he language, profile extended)
;; kb: sk_needed
;; queries: sk_needed
;; profile: extended
(= (sk-needed? $hash $key)
   (not (== (collapse (once (match &sk_needed ($hash $stored $_depth) (if (=alpha $stored $key) true (empty))))) ())))
