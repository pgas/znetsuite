ROOT_DIR ?= $(abspath ..)
MODULE = ztelnet
BUILD_DIR = $(ROOT_DIR)/build/ztelnet
LIB_BUILD_DIR = $(ROOT_DIR)/build/lib
INCLUDE_DIR = $(ROOT_DIR)/include
BIN_DIR = $(ROOT_DIR)/bin

include $(ROOT_DIR)/rules.mk

TARGET = $(BIN_DIR)/ztelnet.com
LIB_TARGET = $(LIB_BUILD_DIR)/libznet.lib
SRCS = ztelnet.c tn_iac.c
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.obj,$(SRCS))

all: $(TARGET)

$(BUILD_DIR)/%.obj: %.c ztelnet.h $(wildcard $(INCLUDE_DIR)/*.h)
	@mkdir -p $(BUILD_DIR)
	@rm -f $@ $(BUILD_DIR)/$*.OBJ
	@ln -sf $(INCLUDE_DIR)/*.h $(BUILD_DIR)/
	@ln -sf $(abspath ztelnet.h) $(BUILD_DIR)/
	@printf "[  C    ] ==> %-30s " "$*.obj"
	@OUT=$$(cd $(BUILD_DIR) && $(ZXCC) $(CC) --C $(HITECHOPTIONS) $(abspath $<) 2>&1); \
	$(call HITECHSTATUS,[ -s $@ ] || [ -s $(BUILD_DIR)/$*.OBJ ])

$(TARGET): $(OBJS) $(LIB_TARGET)
	@mkdir -p $(BIN_DIR)
	@rm -f $(BUILD_DIR)/ztelnet.com $(BUILD_DIR)/ZTELNET.COM
	@printf "[  LNK  ] ==> %-30s " "ztelnet.com"
	@OUT=$$(cd $(BUILD_DIR) && $(ZXCC) $(CC) --V --N ztelnet.obj tn_iac.obj ../lib/libznet.lib 2>&1); \
	$(call HITECHSTATUS,[ -s $(BUILD_DIR)/ztelnet.com ])
	@cp $(BUILD_DIR)/ztelnet.com $(TARGET)
	@echo "Copying ztelnet.com to ../bin"

clean:
	@rm -rf $(BUILD_DIR)

.PHONY: all clean
