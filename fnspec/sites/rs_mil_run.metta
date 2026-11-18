;; fnspec-site: rs_mil_run
;; function: mil:run
;; origin: CeTTa a9b3a811 lib/ilp/metagol_core.metta:133 (stream-data, two nested matches on one space; the file's two other mil:run equations are not part of this site)
;; kb: mil
;; queries: mil
(= (mil:run $sem $program $input)
   (match $sem (MIL.FactPrimitive $program $relation) (match $sem (MIL.Fact $relation $input $output) $output)))
