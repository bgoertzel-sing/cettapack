# match-rows: row hooks, compiled code inside the matcher's row loop

Patch: `0004-match-rows-row-hooks-compiled-code-inside-the-matcher.patch`
Applies to: zariuq/CeTTa a9b3a81186529fd99649e437809bdb6f58fce2fd after 0001, 0002 and 0003 (git am, in order; it
extends 0003's match-rows and 0001's include/cetta_embed_native.h).
Size: 4 files, +296 lines (a new header src/match_rows_hook.h; src/eval.c, src/embedding.c,
include/cetta_embed_native.h).
Status: a file in this repository only; not pushed to CeTTa and not offered upstream (task
t-2026-11-24-cettapack-fnspec-3; stop-line: nothing goes to Zar's CeTTa).

## What it adds

    (match-rows <space> <pattern> <row> (<hook> <arg> ...) <residual>)

The five-argument form runs the same conjunction matcher as 0003's three-argument form. Instead of returning <row>
under each binding row as data, it hands each row (its Bindings, and <row> as the call instantiated it) to a host
hook registered under the symbol <hook>:

- enter(args): once per call, with the hook call's arguments (the host's guards). 1 runs the matcher, 0 takes the
  residual. The hook may ask for the first-row cut (*first_only): match-rows stops at the first kept row.
- query() (optional): the host's own compiled query, patterns and seed Bindings over its own variables, run in
  place of <pattern> (so the host's row code reads each row exactly as it would after its own matcher call).
- row(bindings, row): per binding row, in the matcher's order: 1 keep the answer, 0 drop the row (an (empty)
  branch), 2 no answer, -1 abort the call.
- leave(kept, dropped, aborted): the call's answers from the kept ones (a stream, a collapse tuple, a boolean), or
  0 for the residual.

When enter or leave answers 0, match-rows evaluates <residual> in place of the call (a tail re-entry), so a call the
host refuses answers exactly what <residual> answers. The rows, the hook's per-row work and leave's answers live in a
scratch arena for the call; match-rows copies the answers into the evaluation and frees it. Hooks are registered from the embedding with
cetta_embed_native_rows_hook_register(engine, name, hook) (process-wide, one name one hook, not thread-safe;
cetta_embed_native_rows_hook_unregister before the engine is released). enter runs like a callback (it may call
the engine); row and leave must not.

## Why

cettapack's fnspec compiles whole MeTTa functions (a match under collapse, collapse(once), an existence test, with a
filter or an if/case template) to C on CeTTa's matcher. The compiled function was an embedding callback: every call
goes through the callback cursor, a deep copy of its arguments into an owner arena and a copy of every answer into
a handle. 0003's match-rows avoids that but returns rows as data, so the compiled condition, the per-answer
normal-form check and the first-row cut could not run (cettapack docs/FNSPEC_2.md section 5). With the hook they
run inside the matcher's row loop, behind the same guards. Measured on every fnspec site in cettapack's
docs/FNSPEC_3.md (section 4).

## Semantics note

The hook decides what a row answers; match-rows does not check it. A host must establish, as cettapack's guards do,
that its compiled code answers what CeTTa's evaluation of <residual> would, and refuse (0 or -1) otherwise. The hook
call's arguments are not evaluated (match-rows is a special form); a host that needs values calls it from an
equation whose parameters are bound to the evaluated arguments.
