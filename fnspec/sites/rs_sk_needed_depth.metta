;; fnspec-site: rs_sk_needed_depth
;; function: sk-needed-depth-once
;; origin: PeTTaChainer 33cca343 pettachainer/metta/runtime/evaluator/versions.metta:1247 (once-filter: the (collapse (once ...)) argument of the function at that line, which wraps it in chainer-first-or; the survey has no whole-function once-filter body; profile extended)
;; kb: sk_needed
;; queries: sk_needed
;; profile: extended
(= (sk-needed-depth-once $hash $key)
   (collapse (once (match &sk_needed ($hash $stored $depth) (if (=alpha $stored $key) $depth (empty))))))
