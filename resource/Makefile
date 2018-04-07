.PHONY: all clean

CONFIG_MK = ../config.mk

CC = gcc

INC_DIR = include
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = ../bin
TMP_DIR = ../tmp

CPPFLAGS = $(addprefix -I,$(shell find $(SRC_DIR) -type d))

#CFLAGS   = -g -ggdb -Wall -std=gnu99
CFLAGS   = -O2 -Wall -std=gnu99
LDFLAGS  =

include $(CONFIG_MK)
CFLAGS += $(addprefix -D, $(CONFIG))

PROG = barista_rsm

SRC = $(shell find $(SRC_DIR) -name '*.c')
OBJ = $(patsubst %.c,%.o,$(notdir $(SRC)))
DEP = $(patsubst %.o,.%.dep,$(OBJ))

vpath %.c $(dir $(SRC))
vpath %.o $(OBJ_DIR)

.PRECIOUS: $(OBJ) %.o

all: $(PROG)

$(PROG): $(addprefix $(OBJ_DIR)/,$(OBJ))
	mkdir -p $(@D)
	$(CC) -o $@ $^ $(LDFLAGS)
	mv $(PROG) $(BIN_DIR)

$(OBJ_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/.%.dep: %.c $(CONFIG_MK)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MM $< \
		-MT '$(OBJ_DIR)/$(patsubst .%.dep,%.o,$(notdir $@))' > $@

clean:
	rm -rf $(BIN_DIR)/$(PROG) $(OBJ_DIR) $(INC_DIR)
