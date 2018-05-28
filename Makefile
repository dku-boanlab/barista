.PHONY: all clean

CONFIG_MK = config.mk

CC = gcc

INC_DIR = include
SRC_DIR = src util events components app_events applications
OBJ_DIR = obj
BIN_DIR = bin
LOG_DIR = log
TMP_DIR = tmp

CPPFLAGS = $(addprefix -I,$(shell find $(SRC_DIR) -type d))

#CFLAGS = -O2 -Wall -std=gnu99
CFLAGS = -g -ggdb -Wall -std=gnu99
LDFLAGS = -lpthread -ljansson -lrt -lcli -lzmq

CLUSTER = no
#CLUSTER = yes
ifeq ($(CLUSTER), yes)
	CFLAGS += -I/usr/include/mysql -D__ENABLE_CLUSTER
	LDFLAGS += -L/usr/lib -lmysqlclient
endif

include $(CONFIG_MK)
CFLAGS += $(addprefix -D, $(CONFIG))

PROG = barista

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
	@cd ext_apps/l2_learning_ext; make; cd ../..
	@cd ext_apps/l2_shortest_ext; make; cd ../..

$(OBJ_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/.%.dep: %.c $(CONFIG_MK)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MM $< \
		-MT '$(OBJ_DIR)/$(patsubst .%.dep,%.o,$(notdir $@))' > $@
	mv $(PROG) $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)/$(PROG) $(BIN_DIR)/core $(LOG_DIR)/* $(TMP_DIR)/* $(OBJ_DIR) $(INC_DIR) G*
	@cd ext_apps/l2_learning_ext; make clean; cd ../..
	@cd ext_apps/l2_shortest_ext; make clean; cd ../..
