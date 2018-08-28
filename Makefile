MAKEFLAGS			+=	--no-builtin-rules
.PHONY		: all clean
.SUFFIXES	:

TOP			:= .
TOPi		:= $(abspath $(TOP))
BIN			?= $(TOP)/bin
BINi		:= $(abspath $(BIN))
VERBOSE		?=

ifeq ($(VERBOSE), 1)
   export VV :=
else
   export VV := @
endif

ACCELERATE	:=
CC			:= $(ACCELERATE) clang
CXX			:= $(ACCELERATE) clang++
CFLAGS		?= -g
CXXFLAGS	?= -g
CFLAGSi		:= $(CFLAGS) -std=c11 -Wall -Werror
CXXFLAGSi	:= $(CXXFLAGS) -std=c++11

all			: $(BINi)/morning
clean		:
	rm -rf $(BINi)

$(BINi)/morning				: morning.cpp morning.h
	$(call MAKE_MORNING_TEST)

define MAKE_MORNING_TEST
	@if [ ! -d $(BINi) ]; then mkdir -p $(BINi); fi;
	$(CXX) $(CXXFLAGSi) morning.cpp -o $(BINi)/morning
endef
