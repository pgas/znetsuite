ROOT_DIR ?= $(abspath ..)
MODULE = zgmi
BUILD_DIR = $(ROOT_DIR)/build/zgmi
LIB_BUILD_DIR = $(ROOT_DIR)/build/lib
INCLUDE_DIR = $(ROOT_DIR)/include
BIN_DIR = $(ROOT_DIR)/bin

include $(ROOT_DIR)/rules.mk

TARGET = $(BIN_DIR)/zgmi.com
LIB_TARGET = $(LIB_BUILD_DIR)/libznet.lib
SRCS = zgmi.c ui.c spool.c
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.obj,$(SRCS))

all: $(TARGET)

$(BUILD_DIR)/%.obj: %.c spool.h $(wildcard $(INCLUDE_DIR)/*.h)
	@mkdir -p $(BUILD_DIR)
	@rm -f $@ $(BUILD_DIR)/$*.OBJ
	@ln -sf $(INCLUDE_DIR)/*.h $(BUILD_DIR)/
	@ln -sf $(abspath spool.h) $(BUILD_DIR)/
	@printf "[  C    ] ==> %-30s " "$*.obj"
	@OUT=$$(cd $(BUILD_DIR) && $(ZXCC) $(CC) --C $(HITECHOPTIONS) $(abspath $<) 2>&1); \
	$(call HITECHSTATUS,[ -s $@ ] || [ -s $(BUILD_DIR)/$*.OBJ ])

$(TARGET): $(OBJS) $(LIB_TARGET)
	@mkdir -p $(BIN_DIR)
	@rm -f $(BUILD_DIR)/zgmi.com $(BUILD_DIR)/ZGMI.COM
	@printf "[  LNK  ] ==> %-30s " "zgmi.com"
	@OUT=$$(cd $(BUILD_DIR) && $(ZXCC) $(CC) --V --N zgmi.obj ui.obj spool.obj ../lib/libznet.lib 2>&1); \
	$(call HITECHSTATUS,[ -s $(BUILD_DIR)/zgmi.com ])
	@cp $(BUILD_DIR)/zgmi.com $(TARGET)
	@echo "Copying zgmi.com to ../bin"

clean:
	@rm -rf $(BUILD_DIR)

.PHONY: all clean
