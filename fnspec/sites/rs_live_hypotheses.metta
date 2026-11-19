;; fnspec-site: rs_live_hypotheses
;; function: live-hypotheses
;; origin: PeTTaChainer 33cca343 tests/metta/test_hypothesis_mining.metta:7 (collapse-data; PeTTa type declaration dropped, run in CeTTa's he language)
;; kb: hm_live
;; queries: hm_live
;; setup: &hm_live
(= (live-hypotheses $kb)
   (collapse (match &hm_live ($_hash $kb $consequent $_name $antecedent $_born) (Implication $antecedent $consequent))))
