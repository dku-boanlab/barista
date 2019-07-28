PREFIX = /usr/local

MAJOR = 1
MINOR = 9
REVISION = 8
LIB = libcli.so

CC = gcc
override CFLAGS += -O3 -Wall -std=c99 -pedantic -Wformat-security -Wno-format-zero-length -Werror -Wwrite-strings -Wformat -fdiagnostics-show-option -Wextra -Wsign-compare -Wcast-align -Wno-unused-parameter
override LDFLAGS += -shared
override LIBPATH += -L.
override LDFLAGS += -Wl,-soname,$(LIB).$(MAJOR).$(MINOR)
LIBS = -lcrypt

TARGET_LIBS += $(LIB)

all: $(TARGET_LIBS)

$(LIB): libcli.o
	$(CC) -o $(LIB).$(MAJOR).$(MINOR).$(REVISION) $^ $(LDFLAGS) $(LIBS)
	-rm -f $(LIB) $(LIB).$(MAJOR).$(MINOR)
	ln -s $(LIB).$(MAJOR).$(MINOR).$(REVISION) $(LIB).$(MAJOR).$(MINOR)
	ln -s $(LIB).$(MAJOR).$(MINOR) $(LIB)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -o $@ -c $<

libcli.o: libcli.h

clean:
	rm -f *.o $(LIB)*

install: $(TARGET_LIBS)
	install -d $(PREFIX)/include $(PREFIX)/lib
	install -m 0644 libcli.h $(PREFIX)/include
	install -m 0755 $(LIB).$(MAJOR).$(MINOR).$(REVISION) $(PREFIX)/lib
	cd $(PREFIX)/lib && \
	    ln -fs $(LIB).$(MAJOR).$(MINOR).$(REVISION) $(LIB).$(MAJOR).$(MINOR) && \
	    ln -fs $(LIB).$(MAJOR).$(MINOR) $(LIB)
