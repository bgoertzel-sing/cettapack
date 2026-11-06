; CettaPack M1 match site: variant -> gene -> tissue.
; The kernel is compiled from this region; the residual is its generic match.
(= (cpk-site-target-expressed $space $rs)
   (match $space (, (target $rs $gene) (expressed $gene $tissue)) ($gene $tissue)))
