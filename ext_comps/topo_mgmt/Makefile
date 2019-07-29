.PHONY: all clean

CONFIG_MK = ../../config.mk

CC = gcc

INC_DIR = include
SRC_DIR = src ../shim ../../util
OBJ_DIR = obj
BIN_DIR = ../../bin

CPPFLAGS = $(addprefix -I,$(shell find $(SRC_DIR) -type d))

CFLAGS   = -O2 -Wall -std=gnu99 -I/usr/include/mysql
#CFLAGS   = -g -ggdb -Wall -std=gnu99 -I/usr/include/mysql
LDFLAGS  = -lpthread -ljansson -lcli -lzmq -L/usr/lib -lmysqlclient

include $(CONFIG_MK)
CFLAGS += $(addprefix -D, $(CONFIG))

PROG = topo_mgmt

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
	cp $(PROG) $(BIN_DIR)

$(OBJ_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/.%.dep: %.c $(CONFIG_MK)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MM $< \
	    -MT '$(OBJ_DIR)/$(patsubst .%.dep,%.o,$(notdir $@))' > $@

-include $(DEPS_)
clean:
	rm -rf $(PROG) $(OBJ_DIR) $(INC_DIR)
	rm -f $(PROG) $(BIN_DIR)/$(PROG)
