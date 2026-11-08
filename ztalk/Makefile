ROOT_DIR ?= $(abspath ..)
MODULE = ztalk
BUILD_DIR = $(ROOT_DIR)/build/ztalk
LIB_BUILD_DIR = $(ROOT_DIR)/build/lib
INCLUDE_DIR = $(ROOT_DIR)/include
BIN_DIR = $(ROOT_DIR)/bin

include $(ROOT_DIR)/rules.mk

TARGET = $(BIN_DIR)/ztalk.com
LIB_TARGET = $(LIB_BUILD_DIR)/libznet.lib

all: $(TARGET)

$(BUILD_DIR)/ztalk.obj: ztalk.c $(INCLUDE_DIR)/cio.h $(INCLUDE_DIR)/fmt.h
	@mkdir -p $(BUILD_DIR)
	@ln -sf $(INCLUDE_DIR)/*.h $(BUILD_DIR)/
	@printf "[  C    ] ==> %-30s " "ztalk.obj"
	@OUT=$$(cd $(BUILD_DIR) && $(ZXCC) $(CC) --C $(HITECHOPTIONS) $(abspath $<) 2>&1); \
	$(call HITECHSTATUS,[ -s $@ ] || [ -s $(BUILD_DIR)/ztalk.OBJ ])

$(TARGET): $(BUILD_DIR)/ztalk.obj $(LIB_TARGET)
	@mkdir -p $(BIN_DIR)
	@printf "[  LNK  ] ==> %-30s " "ztalk.com"
	@OUT=$$(cd $(BUILD_DIR) && $(ZXCC) $(CC) --V --N ztalk.obj ../lib/libznet.lib 2>&1); \
	$(call HITECHSTATUS,[ -s $(BUILD_DIR)/ztalk.com ])
	@cp $(BUILD_DIR)/ztalk.com $(TARGET)
	@echo "Copying ztalk.com to ../bin"

clean:
	@rm -rf $(BUILD_DIR)

.PHONY: all clean
