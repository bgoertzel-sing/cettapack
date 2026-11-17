; Option-B site 2: the seed is the second field of (expressed ...), and the
; fan-out is large (hundreds of genes per tissue, several variants per gene).
(= (cpk-site-tissue_fanout $space $tissue)
   (match $space (, (expressed $gene $tissue) (target $rs $gene)) ($rs $gene)))
