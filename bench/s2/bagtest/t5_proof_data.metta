;; S2 bag test t5: data (not definitions) changing in &proof mid-run.  A row added for a pid that already has one,
;; with a different output: proof-output-key-by-id now has two distinct outputs and the compiled code declines
;; (once's pick depends on row order); the row removed: specialised again, same answer.  Data changes do not move
;; the program token, so no revalidation is needed; the code reads &proof live on every call.
!(add-atom &proof (stored-proof-row 301 7 (Inheritance (kb1 ctx ()) (rule p) (STV 1.0 0.7)) k 0.5 () ()))
!(proof-output-key-by-id 301)
!(add-atom &proof (stored-proof-row 301 9 (Implication (kb3 ctx ()) (rule q) (STV 1.0 0.1)) k 0.4 () ()))
!(collapse (proof-output-key-by-id 301))
!(remove-atom &proof (stored-proof-row 301 9 (Implication (kb3 ctx ()) (rule q) (STV 1.0 0.1)) k 0.4 () ()))
!(proof-output-key-by-id 301)
!(remove-atom &proof (stored-proof-row 301 7 (Inheritance (kb1 ctx ()) (rule p) (STV 1.0 0.7)) k 0.5 () ()))
!(collapse (proof-output-key-by-id 301))
