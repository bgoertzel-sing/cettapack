; cettapack native primitives (t-2026-11-26-cettapack-native-primitives): the thin wrapper.
; Run before a PeTTaChainer program, in the same CeTTa process:
;   cetta --lang petta ... bench/np/shim/np_heaps.metta examples/x.metta
; It switches CeTTa's native library(heaps) on (upstream/cetta-np/0010): PeTTaChainer's imported
; Prolog functions empty_heap/1, add_to_heap/4, heap_size/2 and heap_pop/2 (heap_helpers.pl) are
; then answered in C instead of through SWI-Prolog; PeTTaChainer's own files are unchanged.
; Prints nothing (the statement has no result).
!(let $_ (callPredicate (Predicate (cettapack_native_heaps on))) (empty))
