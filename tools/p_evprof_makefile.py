import sys
p=sys.argv[1]
s=open(p).read()
anchor='ifeq ($(ENABLE_PETTA_TYPECHECK_V2),0)\nBUILD_OBJ_TAG := $(BUILD_OBJ_TAG).no-petta-typecheck-v2\nendif\n'
assert s.count(anchor)==1
s=s.replace(anchor,'''# Opt-in evaluator phase timers (src/cetta_evprof.h, cettapack cetta-internals P); off by default.
CETTA_EVPROF ?= 0
ifneq ($(filter $(CETTA_EVPROF),0 1),$(CETTA_EVPROF))
$(error CETTA_EVPROF must be 0 or 1)
endif
EVPROF_CPPFLAGS =
EVPROF_LDFLAGS =
ifeq ($(CETTA_EVPROF),1)
BUILD_OBJ_TAG := $(BUILD_OBJ_TAG).evprof
EVPROF_CPPFLAGS = -DCETTA_EVPROF=1
EVPROF_LDFLAGS = -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free
endif
'''+anchor)
old='$(PROC_FORK_SPAWN_CPPFLAGS) -include $(BUILD_CONFIG_HEADER)'
assert s.count(old)==1
s=s.replace(old,'$(PROC_FORK_SPAWN_CPPFLAGS) $(EVPROF_CPPFLAGS) -include $(BUILD_CONFIG_HEADER)')
i=s.index('\nSRC = src/symbol.c')
j=s.index('\n',i+1)
s=s[:j+1]+'ifeq ($(CETTA_EVPROF),1)\nSRC += src/cetta_evprof.c\nendif\n'+s[j+1:]
old='\t\t$(filter-out FORCE,$^) $(LDFLAGS); \\\n'
assert s.count(old)==1, s.count(old)
s=s.replace(old,'\t\t$(filter-out FORCE,$^) $(LDFLAGS) $(EVPROF_LDFLAGS); \\\n')
open(p,'w').write(s)
