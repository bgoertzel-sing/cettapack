; Option-B site 3: a three-way join; the joined gene is the second field of
; (interacts ...), so that factor's selective field is not first.
(= (cpk-site-variant_interactor $space $rs)
   (match $space (, (target $rs $g1) (interacts $g2 $g1) (expressed $g2 $tissue)) ($g2 $tissue)))
