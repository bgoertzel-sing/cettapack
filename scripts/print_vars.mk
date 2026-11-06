print-tag:
	@echo '$(BUILD_OBJ_TAG)'

print-cpk-link:
	@echo 'CPK_OBJ=$(EMBED_OBJ) $(EMBED_RUNTIME_OBJ)'
	@echo 'CPK_LDFLAGS=$(LDFLAGS)'
	@echo 'CPK_CPPFLAGS=$(CPPFLAGS)'
	@echo 'CPK_CFLAGS=$(CFLAGS)'
	@echo 'CPK_TAG=$(BUILD_OBJ_TAG)'
