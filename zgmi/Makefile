ROOT_DIR ?= $(abspath ..)
MODULE = zgmi
BUILD_DIR = $(ROOT_DIR)/build/zgmi
LIB_BUILD_DIR = $(ROOT_DIR)/build/lib
INCLUDE_DIR = $(ROOT_DIR)/include
BIN_DIR = $(ROOT_DIR)/bin

include $(ROOT_DIR)/rules.mk

TARGET = $(BIN_DIR)/zgmi.com
LIB_TARGET = $(LIB_BUILD_DIR)/libznet.lib

all: $(TARGET)

$(BUILD_DIR)/zgmi.obj: zgmi.c $(INCLUDE_DIR)/cio.h $(INCLUDE_DIR)/term.h $(INCLUDE_DIR)/fmt.h
	@mkdir -p $(BUILD_DIR)
	@ln -sf $(INCLUDE_DIR)/*.h $(BUILD_DIR)/
	@printf "[  C    ] ==> %-30s " "zgmi.obj"
	@OUT=$$(cd $(BUILD_DIR) && $(ZXCC) $(CC) --C $(HITECHOPTIONS) $(abspath $<) 2>&1); \
	$(call HITECHSTATUS,[ -s $@ ] || [ -s $(BUILD_DIR)/zgmi.OBJ ])

$(TARGET): $(BUILD_DIR)/zgmi.obj $(LIB_TARGET)
	@mkdir -p $(BIN_DIR)
	@printf "[  LNK  ] ==> %-30s " "zgmi.com"
	@OUT=$$(cd $(BUILD_DIR) && $(ZXCC) $(CC) --V --N zgmi.obj ../lib/libznet.lib 2>&1); \
	$(call HITECHSTATUS,[ -s $(BUILD_DIR)/zgmi.com ])
	@cp $(BUILD_DIR)/zgmi.com $(TARGET)
	@echo "Copying zgmi.com to ../bin"

clean:
	@rm -rf $(BUILD_DIR)

.PHONY: all clean
