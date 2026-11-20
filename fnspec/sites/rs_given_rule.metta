;; fnspec-site: rs_given_rule
;; function: given-rule?
;; origin: PeTTaChainer 33cca343 pettachainer/metta/runtime/store/rule_identity.metta:21 (exist-data with once; PeTTa type declaration dropped, run in CeTTa's he language, profile extended)
;; kb: given_rules
;; queries: given_rules
;; profile: extended
(= (given-rule? $kbid $name) (not (== (collapse (once (match &given_rules ($name $kbid) true))) ())))
