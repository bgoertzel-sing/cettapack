;; fnspec-site: rs_compound_connective
;; function: compound-connective?
;; origin: PeTTaChainer 33cca343 pettachainer/metta/core/logic_config.metta:49 (exist-data without once; PeTTa type declaration dropped, run in CeTTa's he language, default profile)
;; kb: logic_config
;; queries: logic_config
(= (compound-connective? $head)
   (not (== (collapse (match &logic_config (compound-premise-mode $head $_mode) true)) ())))
