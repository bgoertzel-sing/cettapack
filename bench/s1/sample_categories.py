"""S1: native categories for the sampler's stacks (bench/s1/analyze_samples.py).

A sample's stack is a list of frames, innermost first; each frame a list of function names, innermost inlined
function first.  The category is decided by the innermost function that a rule names, scanning from the leaf
outwards, so a sample in malloc called from the trail code is 'allocation', and a sample in the trail code
itself is 'choice and trail'.  The rules are name patterns over CeTTa a9b3a811's sources (+ 0001-0006); they
were written after reading the top leaf functions of the profiles, and every function the rules do not name
falls to 'other machine' (its share is reported, and the top unnamed functions are listed by the report).
"""
import re

# (category, regex over a function name); first rule matching the innermost function wins
RULES = [
    ("profiler", r"^fnprof_|cetta_fnprof"),
    ("prolog (SWI-Prolog and the foreign boundary)",
     r"@libswipl|^PL_|^pl_|prolog|swipl|_prolog_|foreign_pred|petta_machine_start_predicate"),
    ("allocation and freeing",
     r"^(malloc|calloc|realloc|free|cfree|_int_malloc|_int_free|_int_realloc|malloc_consolidate|"
     r"unlink_chunk|tcache_|sysmalloc|__libc_malloc|__libc_free|__libc_calloc|__libc_realloc|"
     r"arena_alloc|arena_|cetta_malloc|cetta_calloc|cetta_realloc|cetta_free|fallible_)|alloc_|_alloc\b|"
     r"heap_grow|arena_reset|arena_mark|outcome_set_free|_release\b|_destroy\b"),
    ("choice points, trail and backtracking",
     r"choice|trail|backtrack|bindings_builder_save|bindings_builder_restore|binding_mark|undo|"
     r"_rollback|petta_machine_resume|barrier|_cut\b|cut_to"),
    ("equation selection and call preparation",
     r"specializ|prepared_|equation_choice|builtin_equations|callab|program_lookup|open_program|"
     r"refresh_callability|type_call|typecheck|table_safety|petta_table|try_ready_relation|"
     r"start_equation|candidate|rule_head|petta_program_|program_token|foreign_reads"),
    ("copying and instantiation",
     r"apply_bindings|bindings_apply|instantiate|materialize|atom_copy|copy_term|term_copy|deep_copy|"
     r"rename|freshen|fresh_var|fresh_variable|atom_expr\b|atom_expr[0-9]|atom_clone|outcome_atom|resolve_atom|"
     r"substitut|projection_copy|project_reachable|memcpy|memmove|__memcpy|__memmove"),
    ("matching, unification and fact iteration",
     r"match|unify|unif|space_query|native_query|index_lookup|simple_match|bindings_(add|lookup|find|get|bind|"
     r"resolve|frame)|subsum|alpha_eq|atom_eq|structural_eq|oem_|cursor|relation"),
    ("space mutation (add/remove atoms)",
     r"space_add|space_remove|publish|space_insert|space_delete|space_mutation|add_atom|remove_atom|"
     r"revision"),
    ("hashing, interning and term universe", r"hash|intern|symbol_|memcmp|strcmp|__memcmp|term_universe|summary"),
    ("grounded ops and arithmetic", r"grounded|arith|number|_int\b|gmp|__gmp|bigint|float|math"),
    ("goal dispatch and evaluation",
     r"petta_machine_|petta_goal|petta_push|eval_|dispatch|step|petta_plan|equation|metta_"),
]
COMPILED = [(c, re.compile(p)) for c, p in RULES]


def norm(fn):
    """'free@@GLIBC_2.2.5@libc.so.6' -> 'free@libc.so.6' (symbol version dropped)."""
    if "@" not in fn:
        return fn
    parts = fn.split("@")
    return parts[0] + "@" + parts[-1]


def trim(stack):
    """Drop the sampler's own frames: the handler and the signal trampoline (resolved to the nearest exported
    libc symbol, e.g. sigaction) above the interrupted code."""
    i = 0
    while i < len(stack) and stack[i] and stack[i][0].startswith("fnprof_sample_handler"):
        i += 1
    if i and i < len(stack) and stack[i] and ("libc.so" in stack[i][0]):
        i += 1
    return stack[i:]


def leaf_function(stack):
    stack = trim(stack)
    if not stack or not stack[0]:
        return "?"
    return norm(stack[0][0])


def categorise(stack):
    for frame in trim(stack):
        for fn in frame:
            fn = norm(fn)
            for cat, rx in COMPILED:
                if rx.search(fn):
                    return cat
    return "other machine"
