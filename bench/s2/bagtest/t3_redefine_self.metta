;; S2 bag test t3: mid-run changes in &self.  Each block: a call (specialised), a change (the call takes the
;; residual and gives CeTTa's answer under the change), the change undone (respecialised).  Calls go through
;; wrappers (w-...): CeTTa runs a bare top-level call of some of these functions on a route that does not pass the
;; machine's ready-call dispatch (it is then never offered); a wrapper that copies its operand first (copy_term) is
;; run by the machine, and the call inside it is offered.
(= (w-tv $x) (let $c (copy_term $x) (tv-confidence $c)))
(= (w-frs $b $p) (let $c (copy_term $b) (frontier-remaining-score $c $p)))
(= (w-mab $g $a) (let $c (copy_term $a) (make-and-bundle $g $c)))
(= (w-fpp $p) (let $c (copy_term $p) (frontier-pool-potential $c)))
!(w-frs (and-bundle g ((Inheritance (kb1 ctx ()) (rule p) (STV 1.0 0.7)))) ())
!(w-tv (STV 0.5 0.9))
;; 1. a second equation of a compiled callee (tv-confidence): two answers now
!(add-atom &self (= (tv-confidence (STV 0.5 0.9)) 0.42))
!(collapse (w-tv (STV 0.5 0.9)))
!(collapse (w-frs (and-bundle g ((Inheritance (kb1 ctx ()) (rule p) (STV 1.0 0.7)))) ()))
!(remove-atom &self (= (tv-confidence (STV 0.5 0.9)) 0.42))
!(w-tv (STV 0.5 0.9))
!(w-frs (and-bundle g ((Inheritance (kb1 ctx ()) (rule p) (STV 1.0 0.7)))) ())
;; 2. a builtin the code reproduces (min) gets an equation
!(add-atom &self (= (min cpk-never-1 cpk-never-2) 0))
!(w-tv (CTV (STV 1.0 0.3) (STV 0.2 0.8)))
!(remove-atom &self (= (min cpk-never-1 cpk-never-2) 0))
!(w-tv (CTV (STV 1.0 0.3) (STV 0.2 0.8)))
;; 3. a constructor the code builds (and-bundle) gets an equation
!(add-atom &self (= (and-bundle cpk-g cpk-a) cpk-x))
!(w-mab g (a))
!(remove-atom &self (= (and-bundle cpk-g cpk-a) cpk-x))
!(w-mab g (a))
;; 4. a second type declaration of a compiled callee (expr-arity, used by frontier-pool-potential).  A
;; (-> Atom Number) declaration makes CeTTa itself not terminate, in every mode (evidence/s2-2026-11-26/probes/
;; t3_c2); a declaration that unifies with the original, e.g. (-[det]-> Expression Number), is removed together
;; with the original by remove-atom (the guard then rightly stays invalid); this one does neither.
!(add-atom &self (: expr-arity (-> Expression Number Number)))
!(w-fpp ((pool-part k (STV 1.0 0.6) 3 (1) ev mtv)))
!(w-tv (STV 0.5 0.9))
!(remove-atom &self (: expr-arity (-> Expression Number Number)))
!(w-fpp ((pool-part k (STV 1.0 0.6) 3 (1) ev mtv)))
;; 5. a type declaration of a constructor the code dispatches on (STV)
!(add-atom &self (: STV (-> Number Number CpkTV)))
!(w-tv (STV 0.5 0.9))
!(remove-atom &self (: STV (-> Number Number CpkTV)))
!(w-tv (STV 0.5 0.9))
;; 6. the function itself replaced (its one equation removed, another added), then restored
!(remove-atom &self (= (make-and-bundle $goal $args) (and-bundle $goal $args)))
!(add-atom &self (= (make-and-bundle $goal $args) (and-bundle $args $goal)))
!(w-mab g (a))
!(remove-atom &self (= (make-and-bundle $goal $args) (and-bundle $args $goal)))
!(add-atom &self (= (make-and-bundle $goal $args) (and-bundle $goal $args)))
!(w-mab g (a))
;; 7. redefinition inside one evaluation: the call after the add sees the new revision
(= (cpk-redef-then-call)
   (let $_ (add-atom &self (= (tv-confidence (STV 0.25 0.5)) 0.99))
        (collapse (w-tv (STV 0.25 0.5)))))
!(cpk-redef-then-call)
!(remove-atom &self (= (tv-confidence (STV 0.25 0.5)) 0.99))
!(w-tv (STV 0.25 0.5))
