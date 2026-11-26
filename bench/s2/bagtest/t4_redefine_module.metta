;; S2 bag test t4: definitions placed in PeTTaChainer's own module spaces, not &self.  In CeTTa a PeTTa import!
;; loads the chainer's modules into &self itself (no module dependency spaces: the guard trace shows deps 0), so the
;; chainer's own spaces are its data spaces (&proof, &goal, ...).  The guard checks, per revision, every space a
;; compiled function reads: an equation of a compiled function (or its callee) placed in &proof, which
;; proof-output-key-by-id reads, makes every compiled function take the residual (the guard is global) until it is
;; removed (CeTTa does not use equations in &proof for evaluation, so its answers do not change; the guard is
;; conservative there).  A space the code does not read (&goal) does not invalidate anything.
(= (w-pok $p) (let $c (copy_term $p) (proof-output-key-by-id $c)))
(= (w-poa $p) (let $c (copy_term $p) (proof-output-atom-by-id $c)))
(= (w-tv $x) (let $c (copy_term $x) (tv-confidence $c)))
!(add-atom &proof (stored-proof-row 201 7 (Inheritance (kb1 ctx ()) (rule p) (STV 1.0 0.7)) k 0.5 () ()))
!(w-pok 201)
!(w-tv (STV 0.5 0.9))
;; 1. the function itself defined in &proof
!(add-atom &proof (= (proof-output-key-by-id 201) cpk-module-def))
!(w-pok 201)
!(w-poa 201)
!(w-tv (STV 0.5 0.9))
!(remove-atom &proof (= (proof-output-key-by-id 201) cpk-module-def))
!(w-pok 201)
!(w-poa 201)
;; 2. a compiled callee (candidate-output-key) defined in &proof
!(add-atom &proof (= (candidate-output-key $t) cpk-module-def))
!(w-pok 201)
!(remove-atom &proof (= (candidate-output-key $t) cpk-module-def))
!(w-pok 201)
;; (a catch-all equation (= ($f $x) ...) in &proof makes CeTTa itself raise instantiation_error, in every mode;
;; not a usable test)
;; 3. an equation in &goal, a chainer space the compiled code does not read: stays specialised
!(add-atom &goal (= (proof-output-key-by-id 201) cpk-module-def))
!(w-pok 201)
!(remove-atom &goal (= (proof-output-key-by-id 201) cpk-module-def))
!(w-pok 201)
