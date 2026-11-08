ROOT_DIR ?= $(abspath ..)
MODULE = lib
BUILD_DIR = $(ROOT_DIR)/build/lib
INCLUDE_DIR = $(ROOT_DIR)/include

include $(ROOT_DIR)/rules.mk

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.obj,$(SRCS))
LIB_TARGET = $(BUILD_DIR)/libznet.lib

all: $(LIB_TARGET)

$(BUILD_DIR)/%.obj: %.c $(wildcard $(INCLUDE_DIR)/*.h)
	@mkdir -p $(BUILD_DIR)
	@ln -sf $(INCLUDE_DIR)/*.h $(BUILD_DIR)/
	@printf "[  C    ] ==> %-30s " "$*.obj"
	@OUT=$$(cd $(BUILD_DIR) && $(ZXCC) $(CC) --C $(HITECHOPTIONS) $(abspath $<) 2>&1); \
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
