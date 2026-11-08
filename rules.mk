#
# ZNetSuite - Common Build Rules for HI-TECH C & RomWBW
#

ROMWBW ?= $(abspath $(ROOT_DIR)/../RomWBW)
TOOLS ?= $(ROMWBW)/Tools
UNAME := $(shell uname)
BINDIR = $(TOOLS)/$(UNAME)
ZXCC = $(BINDIR)/zxcc

HITECHDIR = $(ROMWBW)/Source/Images/d_hitechc/u0
CC = $(HITECHDIR)/C
LIBR = $(HITECHDIR)/LIBR
HITECHOPTIONS = --N

export CPMDIR80 = $(TOOLS)/cpm/
export BINDIR80 = $(HITECHDIR)
export LIBDIR80 = $(HITECHDIR)
export INCDIR80 = $(HITECHDIR)

GREEN = \033[32m
RED   = \033[31m
NC    = \033[0m

define HITECHSTATUS
	if $(1); then \
		printf "[ $(GREEN)OK$(NC) ]\n"; \
	else \
		printf "[ $(RED)FAIL$(NC) ]\n"; \
		echo "$$OUT"; \
		exit 1; \
	fi
endef

