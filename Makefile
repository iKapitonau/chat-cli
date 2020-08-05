CC ?= gcc
LDFLAGS := $(LDFLAGS) -lncurses
CFLAGS := $(CFLAGS) -Wall -Wextra -pedantic -O3 -pthread -Iinclude/

export obj-server obj-client obj-common
export deps

client := src/client
server := src/server
common := src/common

include $(client)/Makefile
include $(server)/Makefile
include $(common)/Makefile

.PHONY: all clean cleandeps

all: server client

server: $(obj-server) $(obj-common)
	$(CC) $(obj-server) $(obj-common) $(CFLAGS) $(LDFLAGS) -o $@ 

client: $(obj-client) $(obj-common)
	$(CC) $(obj-client) $(obj-common) $(CFLAGS) $(LDFLAGS) -o $@ 

include $(deps)

%.dep: %.c
	$(CC) $(CFLAGS) $< -MM > $@

clean:
	-rm -f $(obj-server) $(obj-common) $(obj-client) server client

cleandeps:
	-rm -f $(deps)
