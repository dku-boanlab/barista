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
	@cd ext_comps/conn; make; cd ../..
	@cd ext_comps/ofp10; make; cd ../..
	@cd ext_comps/switch_mgmt; make; cd ../..
	@cd ext_comps/host_mgmt; make; cd ../..
	@cd ext_comps/topo_mgmt; make; cd ../..
	@cd ext_comps/flow_mgmt; make; cd ../..
	@cd ext_comps/stat_mgmt; make; cd ../..
	@cd ext_comps/resource_mgmt; make; cd ../..
	@cd ext_comps/traffic_mgmt; make; cd ../..
	@cd ext_comps/cac; make; cd ../..
	@cd ext_comps/dit; make; cd ../..
	@cd ext_comps/ofp10_veri; make; cd ../..
	@cd ext_comps/conflict; make; cd ../..
	@cd ext_apps/l2_learning; make; cd ../..
	@cd ext_apps/l2_shortest; make; cd ../..
	@cd ext_apps/rbac; make; cd ../..

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
	@cd ext_comps/conn; make clean; cd ../..
	@cd ext_comps/ofp10; make clean; cd ../..
	@cd ext_comps/switch_mgmt; make clean; cd ../..
	@cd ext_comps/host_mgmt; make clean; cd ../..
	@cd ext_comps/topo_mgmt; make clean; cd ../..
	@cd ext_comps/flow_mgmt; make clean; cd ../..
	@cd ext_comps/stat_mgmt; make clean; cd ../..
	@cd ext_comps/resource_mgmt; make clean; cd ../..
	@cd ext_comps/traffic_mgmt; make clean; cd ../..
	@cd ext_comps/cac; make clean; cd ../..
	@cd ext_comps/dit; make clean; cd ../..
	@cd ext_comps/ofp10_veri; make clean; cd ../..
	@cd ext_comps/conflict; make clean; cd ../..
	@cd ext_apps/l2_learning; make clean; cd ../..
	@cd ext_apps/l2_shortest; make clean; cd ../..
	@cd ext_apps/rbac; make clean; cd ../..
