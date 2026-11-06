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
            -DCPK_BUILD_MODE='"$(CPK_BUILD_MODE)"' -DCPK_CETTA_ROOT='"$(C)"' \
            -DCPK_PACK_DIR='"$(CURDIR)/packs/target_expressed"'
CFLAGS_CPK := -O2 -Wall -Werror -Wno-format-truncation -std=c11 -pthread
LINK_OBJ := $(addprefix $(C)/,$(CPK_OBJ))
LINK_LD := $(patsubst runtime/%,$(C)/runtime/%,$(CPK_LDFLAGS))

SRC := src/cpk_host.c src/cpk_pack.c src/cpk_kernel.c src/cpk_support.c packs/target_expressed/kernel_plan.c
HDR := src/cpk.h src/cpk_pack.h src/cpk_support.h

all: build/cettapack-host

packs/target_expressed/kernel_plan.c packs/target_expressed/manifest.json: packs/target_expressed/site.metta tools/cpkc.py
	python3 tools/cpkc.py packs/target_expressed/site.metta --out-dir packs/target_expressed

build/cettapack-host: $(SRC) $(HDR) build/cetta.mk packs/target_expressed/manifest.json
	@test -n "$(C)" || (echo "run scripts/build_cetta.sh first" && false)
	gcc $(CFLAGS_CPK) $(CPK_INC) $(CPK_DEFS) -o $@ $(SRC) $(LINK_OBJ) $(LINK_LD)

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

bench: build/cettapack-host build/kb.metta
	bench/run_bench.sh

clean:
	rm -f build/cettapack-host

.PHONY: all test bench clean
