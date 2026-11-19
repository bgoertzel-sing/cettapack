;; fnspec-site: rs_base_rate_rows
;; function: base-rate-cache-mode-rows
;; origin: PeTTaChainer 33cca343 pettachainer/metta/runtime/statistics/base_rates.metta:41 (collapse-filter; PeTTa type declaration dropped, run in CeTTa's he language)
;; kb: base_rate
;; queries: base_rate
;; setup: &ctx_base_rate_cache
(= (base-rate-cache-mode-rows $hash $kbid $pattern $mode)
   (collapse
      (match &ctx_base_rate_cache ($hash (base-rate $kbid $stored $tv $source $emode))
         (if (and (=alpha $stored $pattern) (or (== $emode any) (== $emode $mode)))
            (base-rate $kbid $stored $tv $source $emode)
            (empty)))))
