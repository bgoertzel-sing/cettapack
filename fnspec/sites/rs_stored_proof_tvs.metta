;; fnspec-site: rs_stored_proof_tvs
;; function: stored-proof-tvs
;; origin: PeTTaChainer 33cca343 pettachainer/metta/runtime/proofs.metta:150 (once-data; PeTTa type declaration dropped, run in CeTTa's he language, profile extended)
;; kb: kb_proofs
;; queries: kb_proofs
;; profile: extended
(= (stored-proof-tvs $name) (collapse (once (match &kb ($type $kbctx $name $tv) $tv))))
