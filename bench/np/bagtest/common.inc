; Heap bag tests (cettapack native primitives): the test program imports PeTTaChainer exactly as its examples do
; (the file sits one level under the chainer root, so heap_helpers.pl's heap_pop is found), then runs heap
; operation sequences through PeTTaChainer's own heap imports (empty_heap, add_to_heap, heap_size, heap_pop).
!(import! &self ../petta_chainer)
