
def patch(f, pairs):
    s = open(f).read()
    for old, new in pairs:
        assert s.count(old) == 1, (f, old[:90])
        s = s.replace(old, new)
    open(f, 'w').write(s)
C = 'build/cetta-src/'
patch(C + 'src/match_rows_hook.h', [
(""" *   row    per binding row""", """ *   query  optional (NULL: <pattern>): the query the host compiled, as patterns and seed
 *          bindings whose variables are the host's own (so its row code reads them directly);
 *          match-rows runs the same conjunction matcher on it;
 *   row    per binding row"""),
("""    int (*row)(void *context, void *call, Arena *arena, Bindings *bindings, Atom *row, Atom **answer);""",
 """    int (*query)(void *context, void *call, Atom *const **patterns, size_t *npatterns, const Bindings **seed);
    int (*row)(void *context, void *call, Arena *arena, Bindings *bindings, Atom *row, Atom **answer);"""),
])
patch(C + 'src/eval.c', [
("""        if (ms) {
            BindingSet rows;
            if (pattern->kind == ATOM_EXPR && pattern->expr.len >= 2u &&""",
 """        Atom *const *hook_pats = NULL;
        size_t hook_npats = 0;
        const Bindings *hook_seed = NULL;
        if (ms && hook->query &&
            hook->query(hook->context, hook_state, &hook_pats, &hook_npats, &hook_seed) != 1)
            aborted = 1;
        if (ms && !aborted) {
            BindingSet rows;
            if (hook_pats) {
                space_query_conjunction(ms, &scratch, (Atom **)hook_pats,
                                        (CettaExprLen)hook_npats, hook_seed, &rows);
            } else if (pattern->kind == ATOM_EXPR && pattern->expr.len >= 2u &&"""),
])
patch(C + 'include/cetta_embed_native.h', [
("""    int (*row)(void *context, void *call, void *arena, void *bindings, struct Atom *row, struct Atom **answer);""",
 """    /* optional: the query the host compiled (patterns and seed Bindings over its own variables);
     * NULL runs <pattern> as the call instantiated it.  1 ok, else the call aborts (residual). */
    int (*query)(void *context, void *call, struct Atom *const **patterns, size_t *npatterns, const void **seed);
    int (*row)(void *context, void *call, void *arena, void *bindings, struct Atom *row, struct Atom **answer);"""),
])
patch(C + 'src/embedding.c', [
("""static int embedding_rows_leave(""", """static int embedding_rows_query(void *context, void *call, Atom *const **patterns, size_t *npatterns,
                                const Bindings **seed) {
    EmbeddingRowsHook *hook = context;
    return hook->user.query(hook->user.context, call, patterns, npatterns, (const void **)seed);
}
static int embedding_rows_leave("""),
("""    CettaRowsHook internal = {.context = wrapper, .enter = embedding_rows_enter,
                              .row = embedding_rows_row, .leave = embedding_rows_leave};""",
 """    CettaRowsHook internal = {.context = wrapper, .enter = embedding_rows_enter,
                              .query = hook->query ? embedding_rows_query : NULL,
                              .row = embedding_rows_row, .leave = embedding_rows_leave};"""),
])
# diag host copy
D = '/tmp/f3diag/src/'
patch(D + 'cpk_fnspec.c', [
("typedef struct { FsRt *rt; Atom *args[16]; uint64_t t0; } RowsCall;", "typedef struct { FsRt *rt; Atom *args[16]; uint64_t t0; Bindings sb; bool sb_init; } RowsCall;"),
("""    c->rt = rt;
    c->t0 = dnow();""", """    c->rt = rt;
    c->t0 = dnow();
    c->sb_init = false;
    if (getenv("CPK_ROWS_QUERY")) {
        bindings_init(&c->sb);
        c->sb_init = true;
        for (unsigned i = 0; i < p->arity; i++)
            if ((int)i != p->space_arg && !bindings_add_var(&c->sb, rt->lhs_args[i], (Atom *)args[i])) { bindings_free(&c->sb); free(c); rt->st.row_aborts++; rt->st.residual_calls++; return 0; }
    }"""),
("""    FsCall fc = {rt, c->args, row->expr.elems};
    return p->row(&fc, bindings, arena, answer);""", """    FsCall fc = {rt, c->args, c->sb_init ? NULL : row->expr.elems};
    return p->row(&fc, bindings, arena, answer);"""),
("""    rt->st.inner_ns += dnow() - c->t0;
    free(c);""", """    rt->st.inner_ns += dnow() - c->t0;
    if (c->sb_init) bindings_free(&c->sb);
    free(c);"""),
("""static int rows_leave(""", """static int rows_query(void *ctx, void *call, struct Atom *const **patterns, size_t *npatterns, const void **seed) {
    (void)ctx;
    RowsCall *c = call;
    if (!c->sb_init) { *patterns = NULL; *npatterns = 0; *seed = NULL; return 1; }
    *patterns = c->rt->pats; *npatterns = c->rt->plan->n_patterns; *seed = &c->sb;
    return 1;
}

static int rows_leave("""),
("""    cetta_rows_hook_t h = {.size = sizeof h, .context = rt, .enter = rows_enter, .row = rows_row, .leave = rows_leave};""",
 """    cetta_rows_hook_t h = {.size = sizeof h, .context = rt, .enter = rows_enter, .query = rows_query, .row = rows_row, .leave = rows_leave};"""),
])
print('ok')
