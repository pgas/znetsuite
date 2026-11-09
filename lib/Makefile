ROOT_DIR ?= $(abspath ..)
MODULE = lib
BUILD_DIR = $(ROOT_DIR)/build/lib
INCLUDE_DIR = $(ROOT_DIR)/include

include $(ROOT_DIR)/rules.mk

# Ordered topologically (high-level callers before low-level leaf primitives)
# for HI-TECH C's single-pass LINQ library resolver.
SRCS = trm_scrl.c trm_goto.c trm_size.c trm_key.c trm_base.c \
       hb_list.c hb_det.c hb_cnt.c hb_dev.c hb_name.c \
       fmt_cpm.c fmt_size.c fmt_pnum.c fmt_phex.c fmt_pint.c \
       fmt_int.c fmt_uint.c fmt_hex.c fmt_hx16.c fmt_trim.c \
       fmt_case.c fmt_upr.c \
       cpm_fcb.c cpm_bdos.c cpm_exit.c \
       cio_cmd.c cio_ok.c cio_line.c cio_to.c cio_flsh.c \
       cio_prtf.c cio_puts.c cio_wri.c \
       cio_in.c cio_out.c cio_ist.c cio_ost.c
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.obj,$(SRCS))
LIB_TARGET = $(BUILD_DIR)/libznet.lib

all: $(LIB_TARGET)

$(BUILD_DIR)/%.obj: %.c $(wildcard $(INCLUDE_DIR)/*.h)
	@mkdir -p $(BUILD_DIR)
	@rm -f $@ $(BUILD_DIR)/$*.OBJ
	@ln -sf $(INCLUDE_DIR)/*.h $(BUILD_DIR)/
	@printf "[  C    ] ==> %-30s " "$*.obj"
	@OPT="$(HITECHOPTIONS)"; \
	if grep -q '#asm' $<; then OPT="$(HITECHASMOPTIONS)"; fi; \
	OUT=$$(cd $(BUILD_DIR) && $(ZXCC) $(CC) --C $$OPT $(abspath $<) 2>&1); \
	$(call HITECHSTATUS,[ -s $@ ] || [ -s $(BUILD_DIR)/$*.OBJ ])

$(LIB_TARGET): $(OBJS)
	@printf "[  LIB  ] ==> %-30s " "libznet.lib"
	@rm -f $@
	@OUT=$$(cd $(BUILD_DIR) && for obj in $(notdir $(OBJS)); do \
		$(ZXCC) $(LIBR) -r -libznet.lib -$$obj 2>&1 || exit 1; \
	done); \
	$(call HITECHSTATUS,[ -s $@ ])

clean:
	@rm -rf $(BUILD_DIR)

.PHONY: all clean
