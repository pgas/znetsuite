ROOT_DIR ?= $(abspath ..)
MODULE = lib
BUILD_DIR = $(ROOT_DIR)/build/lib
INCLUDE_DIR = $(ROOT_DIR)/include

include $(ROOT_DIR)/rules.mk

OBJS = $(BUILD_DIR)/cio.obj $(BUILD_DIR)/term.obj $(BUILD_DIR)/fmt.obj $(BUILD_DIR)/cpm.obj

all: $(OBJS)

$(BUILD_DIR)/%.obj: %.c $(wildcard $(INCLUDE_DIR)/*.h)
	@mkdir -p $(BUILD_DIR)
	@ln -sf $(INCLUDE_DIR)/*.h $(BUILD_DIR)/
	@ln -sf $(ROOT_DIR)/lib/*.c $(BUILD_DIR)/
	@printf "[  C    ] ==> %-30s " "$*.obj"
	@OUT=$$(cd $(BUILD_DIR) && $(ZXCC) $(CC) --C $(HITECHOPTIONS) $(abspath $<) 2>&1); \
	$(call HITECHSTATUS,[ -s $@ ] || [ -s $(BUILD_DIR)/$*.OBJ ])

clean:
	@rm -rf $(BUILD_DIR)

.PHONY: all clean
