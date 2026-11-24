
import os, re, shutil
D='/tmp/f3diag'
shutil.rmtree(D, ignore_errors=True)
os.makedirs(D)
shutil.copytree('src', D + '/src')
shutil.copytree('build/fnspec', D + '/gen')
def patch(f, pairs):
    s = open(f).read()
    for old, new in pairs:
        assert s.count(old) == 1, (f, old[:90])
        s = s.replace(old, new)
    open(f, 'w').write(s)
patch(D + '/src/cpk_fnspec.h', [
("    uint64_t hosted_calls;\n", "    uint64_t hosted_calls;\n    uint64_t inner_ns;   /* diag */\n"),
])
patch(D + '/src/cpk_fnspec.c', [
('#include <string.h>\n', '#include <string.h>\n#include <time.h>\nstatic uint64_t dnow(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec; }\n'),
("typedef struct { FsRt *rt; Atom *args[16]; } RowsCall;", "typedef struct { FsRt *rt; Atom *args[16]; uint64_t t0; } RowsCall;"),
("    c->rt = rt;\n    for (unsigned i = 0; i < p->arity; i++) c->args[i] = (Atom *)args[i];", "    c->rt = rt;\n    c->t0 = dnow();\n    for (unsigned i = 0; i < p->arity; i++) c->args[i] = (Atom *)args[i];"),
("    Arena *a = arena;\n    free(c);", "    Arena *a = arena;\n    rt->st.inner_ns += dnow() - c->t0;\n    free(c);"),
("            uint64_t rejects = rt->st.arg_rejects;\n            spec = compute(c);", "            uint64_t rejects = rt->st.arg_rejects;\n            uint64_t t0 = dnow();\n            spec = compute(c);\n            rt->st.inner_ns += dnow() - t0;"),
("""        if (!ok) { *error = "load the cpk-rows definition"; return NULL; }
    }""", """        if (!ok) { *error = "load the cpk-rows definition"; return NULL; }
        /* diag: 0003's three-argument form on the same pattern and row */
        const char *hk = strstr(p->rows_def, " (CPK_ROWS_HOOK");
        const char *nm = strstr(p->rows_def, "cpk-rows:");
        char *src3 = fmt_s("%.*scpk-rows3:%.*s))", (int)(nm - p->rows_def), p->rows_def, (int)(hk - nm - 9), nm + 9);
        r = NULL;
        ok = cetta_embed_run(e, src3, strlen(src3), NULL, &r) == CETTA_OK && cetta_embed_result_status(r) == CETTA_OK;
        cetta_embed_result_release(r);
        free(src3);
        if (!ok) { *error = "rows3"; return NULL; }
    }"""),
])
patch(D + '/src/cpk_fnspec_host.c', [
('{"fn-rows", false}};', '{"fn-rows", false}, {"fn-rows3", false}};'),
('    else if (!strcmp(m, "fn-rows")) snprintf(buf, n, "cpk-rows:%s", v->p->function);', '    else if (!strcmp(m, "fn-rows")) snprintf(buf, n, "cpk-rows:%s", v->p->function);\n    else if (!strcmp(m, "fn-rows3")) snprintf(buf, n, "cpk-rows3:%s", v->p->function);'),
(',\\"guard_valid\\":%s",', ',\\"inner_ns\\":%" PRIu64 ",\\"guard_valid\\":%s",'),
('             s->revalidations, s->hosted_calls, rt->valid ? "true" : "false");', '             s->revalidations, s->hosted_calls, s->inner_ns, rt->valid ? "true" : "false");'),
])
print('ok')
