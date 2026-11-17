; Option-B site 1: the seed is the second field of (target ...), not the first.
; gene -> its variants and the tissues it is expressed in.
(= (cpk-site-gene_variants $space $gene)
   (match $space (, (target $rs $gene) (expressed $gene $tissue)) ($rs $tissue)))
