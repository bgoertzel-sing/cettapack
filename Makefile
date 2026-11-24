# CettaPack M1. First: scripts/build_cetta.sh (writes build/cetta.mk).
-include build/cetta.mk

PROFILE := $(shell python3 tools/cpkc.py --print-profile-components)
CPK_CETTA_COMMIT := $(patsubst CPK_CETTA_COMMIT=%,%,$(filter CPK_CETTA_COMMIT=%,$(PROFILE)))
CPK_PATCHES_SHA256 := $(patsubst CPK_PATCHES_SHA256=%,%,$(filter CPK_PATCHES_SHA256=%,$(PROFILE)))
CPK_BUILD_MODE := $(patsubst CPK_BUILD_MODE=%,%,$(filter CPK_BUILD_MODE=%,$(PROFILE)))

C := $(CPK_CETTA_ROOT)
# CeTTa's include flags are relative to its tree
CPK_INC := $(foreach f,$(filter -I%,$(CPK_CPPFLAGS)),$(if $(filter -I/%,$(f)),$(f),-I$(C)/$(patsubst -I%,%,$(f)))) $(filter -D%,$(CPK_CPPFLAGS)) \
           -include $(C)/$(lastword $(CPK_CPPFLAGS)) -I$(C)/include -Isrc
CPK_DEFS := -DCPK_CETTA_COMMIT='"$(CPK_CETTA_COMMIT)"' -DCPK_PATCHES_SHA256='"$(CPK_PATCHES_SHA256)"' \
            -DCPK_BUILD_MODE='"$(CPK_BUILD_MODE)"' -DCPK_CETTA_ROOT='"$(C)"'
CFLAGS_CPK := -O2 -Wall -Werror -Wno-format-truncation -std=c11 -pthread
LINK_OBJ := $(addprefix $(C)/,$(CPK_OBJ))
LINK_LD := $(patsubst runtime/%,$(C)/runtime/%,$(CPK_LDFLAGS))

SRC := src/cpk_host.c src/cpk_pack.c src/cpk_kernel.c src/cpk_support.c
HDR := src/cpk.h src/cpk_pack.h src/cpk_support.h

# M1's site plus the option-B sites (docs/SITES.md); one host binary per site.
SITES := target_expressed gene_variants tissue_fanout variant_interactor
HOSTS := $(addprefix build/cettapack-host-,$(SITES))

all: build/cettapack-host $(HOSTS)

packs/%/kernel_plan.c packs/%/manifest.json: packs/%/site.metta tools/cpkc.py upstream/cetta/*.patch
	python3 tools/cpkc.py packs/$*/site.metta --out-dir packs/$*

.SECONDARY: $(foreach s,$(SITES),packs/$(s)/kernel_plan.c packs/$(s)/manifest.json)

build/cettapack-host-%: $(SRC) $(HDR) build/cetta.mk packs/%/manifest.json packs/%/kernel_plan.c
	@test -n "$(C)" || (echo "run scripts/build_cetta.sh first" && false)
	@echo "gcc ... -o $@ (site $*)"
	@gcc $(CFLAGS_CPK) $(CPK_INC) $(CPK_DEFS) -DCPK_PACK_DIR='"$(CURDIR)/packs/$*"' \
	    -DCPK_PLAN_SYM=cpk_plan_cpk_site_$* -o $@ $(SRC) packs/$*/kernel_plan.c $(LINK_OBJ) $(LINK_LD)

# Next-step probes (docs/NEXT_STEP_PROBES.md): the same pack sources, cpk_probe.c as the host
PROBE_SRC := src/cpk_probe.c src/cpk_pack.c src/cpk_kernel.c src/cpk_support.c
PROBES := $(addprefix build/cettapack-probe-,$(SITES))

build/cettapack-probe-%: $(PROBE_SRC) $(HDR) build/cetta.mk packs/%/manifest.json packs/%/kernel_plan.c
	@test -n "$(C)" || (echo "run scripts/build_cetta.sh first" && false)
	@echo "gcc ... -o $@ (probe host, site $*)"
	@gcc $(CFLAGS_CPK) $(CPK_INC) $(CPK_DEFS) -DCPK_PACK_DIR='"$(CURDIR)/packs/$*"' \
	    -DCPK_PLAN_SYM=cpk_plan_cpk_site_$* -o $@ $(PROBE_SRC) packs/$*/kernel_plan.c $(LINK_OBJ) $(LINK_LD)

probes: $(PROBES)


# The next-step probes' bag tests (docs/NEXT_STEP_PROBES.md), unchanged: the four
# probe sites' answers in every probe mode, so that this milestone leaves them as they were
probe-difftest: $(PROBES) build/kb.metta
	@set -e; for s in $(SITES); do \
	  python3 bench/gen_kb.py --query-kind $$(case $$s in gene_variants) echo gene;; tissue_fanout) echo tissue;; *) echo rs;; esac) --out build/kb-pdiff.metta --queries-out build/queries-pdiff-$$s.txt >/dev/null; \
	  cmp -s build/kb-pdiff.metta build/kb.metta; \
	  ./build/cettapack-probe-$$s difftest --kb build/kb.metta --queries build/queries-pdiff-$$s.txt > build/probe-difftest-$$s.txt 2>&1 || { cat build/probe-difftest-$$s.txt; exit 1; }; \
	  tail -1 build/probe-difftest-$$s.txt; \
	done

# Whole-function specialisation (docs/FNSPEC.md): one generated C file per site,
# tools/fnspecc.py; one host for every site
FNSPEC_SITES := $(sort $(notdir $(basename $(wildcard fnspec/sites/*.metta))))
FNSPEC_GEN := $(addprefix build/fnspec/,$(addsuffix .c,$(FNSPEC_SITES)))
FNSPEC_SRC := src/cpk_fnspec.c src/cpk_fnspec_host.c src/cpk_support.c

build/fnspec/%.c: fnspec/sites/%.metta tools/fnspecc.py tools/survey_shapes.py
	@mkdir -p build/fnspec
	python3 tools/fnspecc.py $< --out $@

build/fnspec/registry.c: $(wildcard fnspec/sites/*.metta) tools/fnspecc.py
	@mkdir -p build/fnspec
	python3 tools/fnspecc.py --registry $@ $(addprefix fnspec/sites/,$(addsuffix .metta,$(FNSPEC_SITES)))

build/cettapack-fnspec: $(FNSPEC_SRC) src/cpk_fnspec.h src/cpk_support.h $(FNSPEC_GEN) build/fnspec/registry.c build/cetta.mk
	@test -n "$(C)" || (echo "run scripts/build_cetta.sh first" && false)
	@echo "gcc ... -o $@ ($(words $(FNSPEC_SITES)) fnspec sites)"
	@gcc $(CFLAGS_CPK) -Wno-unused-function $(CPK_INC) $(CPK_DEFS) -o $@ $(FNSPEC_SRC) $(FNSPEC_GEN) build/fnspec/registry.c $(LINK_OBJ) $(LINK_LD)

fnspec: build/cettapack-fnspec

# Bag tests for every fnspec site: small spaces (tests/fnspec_cases.py), the
# mutation sequence and recursion on the first case, then the site's benchmark
# KB and queries; every mode against CeTTa evaluating the function.
fnspec-difftest: build/cettapack-fnspec build/kb.metta
	@mkdir -p build/fnspec-cases build/fnspec-kb
	python3 tests/fnspec_cases.py --out build/fnspec-cases
	@set -e; for s in $(FNSPEC_SITES); do \
	  args=$$(python3 bench/fnspec_inputs.py --site $$s --build build); \
	  kind=$$(python3 bench/fnspec_inputs.py --site $$s --cases-kind); \
	  ./build/cettapack-fnspec difftest --site $$s --cases build/fnspec-cases/$$kind $$args > build/fnspec-difftest-$$s.txt 2>&1 || { cat build/fnspec-difftest-$$s.txt; exit 1; }; \
	  tail -1 build/fnspec-difftest-$$s.txt; \
	done

# M1's binary name, kept for the M1 commands below
build/cettapack-host: build/cettapack-host-target_expressed
	cp $< $@

build/kb.metta build/queries.txt: bench/gen_kb.py
	python3 bench/gen_kb.py --out build/kb.metta --queries-out build/queries.txt

HE_KB_ARGS := --snps 240 --genes 60 --interactions 300 --annotations 200 --queries 40

build/kb-small.metta build/queries-small.txt: bench/gen_kb.py
	python3 bench/gen_kb.py $(HE_KB_ARGS) --out build/kb-small.metta --queries-out build/queries-small.txt

# One command for every M1 check (A.9): hashes, differential tests, the HE
# comparison (item 16), the cross-space refusal (item 8), the certificate.
test: build/cettapack-host build/kb.metta build/kb-small.metta
	./build/cettapack-host hashes
	CPK_TEST_LOG=build/test-results.jsonl ./build/cettapack-host test --kb build/kb.metta
	python3 tests/he_diff.py --host build/cettapack-host --kb build/kb-small.metta --queries build/queries-small.txt | tee build/test-he.txt
	! python3 tools/cpkc.py tests/cross_space_site.metta --out-dir build/cross-space 2>build/test-cross.txt; cat build/test-cross.txt
	python3 tools/emit_certificate.py packs/target_expressed build/test-results.jsonl build/kb.metta build/test-he.txt
	$(MAKE) -s difftest
	$(MAKE) -s probe-difftest
	$(MAKE) -s fnspec-difftest

# Differential bag tests on every site (bag and support demand on small
# spaces, then the benchmark KB with the site's own seeds): kernel against
# generic CeTTa, every call on the kernel path.
seeds_kind = $(if $(filter gene_variants,$(1)),gene,$(if $(filter tissue_fanout,$(1)),tissue,rs))
difftest: $(HOSTS) build/kb.metta
	@set -e; $(foreach s,$(SITES),\
	  python3 bench/gen_kb.py --query-kind $(call seeds_kind,$(s)) --out build/kb-diff.metta --queries-out build/queries-diff-$(s).txt >/dev/null; \
	  cmp -s build/kb-diff.metta build/kb.metta; \
	  ./build/cettapack-host-$(s) difftest --kb build/kb.metta --queries build/queries-diff-$(s).txt > build/difftest-$(s).txt 2>&1 || { cat build/difftest-$(s).txt; exit 1; }; \
	  cat build/difftest-$(s).txt;)

bench: build/cettapack-host build/kb.metta
	bench/run_bench.sh

clean:
	rm -f build/cettapack-host

.PHONY: all test bench clean difftest probes probe-difftest fnspec fnspec-difftest fnspec-recompute fnspec3-recompute

# Recompute the benchmark tables from the raw runs and compare with the committed summary
# (fnspec 1 and fnspec 2 evidence; one summariser for both)
FNSPEC_BENCH ?= evidence/fnspec-2026-11-18/bench evidence/fnspec2-2026-11-19/bench evidence/fnspec3-2026-11-24/bench
fnspec-recompute:
	@mkdir -p build
	@set -e; for d in $(FNSPEC_BENCH); do \
	  python3 bench/summarize_fnspec.py $$d > build/fnspec-summary-recomputed.md; \
	  cmp build/fnspec-summary-recomputed.md $$d/summary.md && echo "recomputed tables match $$d/summary.md"; \
	done
	@$(MAKE) -s fnspec3-recompute

# fnspec 3, item 3: match-rows' row hook (0004) vs the embedding callback on every site
# (the fnspec 1/2 sites in hook-bench, the fnspec 3 sites in bench)
FNSPEC3_HOOK ?= evidence/fnspec3-2026-11-24/hook-bench evidence/fnspec3-2026-11-24/bench
fnspec3-recompute:
	@mkdir -p build
	@python3 bench/summarize_rows_hook.py $(FNSPEC3_HOOK) > build/rows-hook-recomputed.md
	@cmp build/rows-hook-recomputed.md evidence/fnspec3-2026-11-24/rows-hook.md && echo "recomputed tables match evidence/fnspec3-2026-11-24/rows-hook.md"
