; Not compilable in M1: a cross-space join (A.9 item 8). cpkc must refuse it.
(= (cpk-site-cross $a $b $rs)
   (match $a (target $rs $gene) (match $b (expressed $gene $tissue) ($gene $tissue))))
