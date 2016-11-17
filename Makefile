TARGETS = conntrack-flush route-monitor

all: $(TARGETS)

clean:
	rm -f *.o $(TARGETS)

PREFIX ?= /usr/local

install: $(TARGETS)
	install -D -d $(DESTDIR)/$(PREFIX)/bin
	install -s -m 0755 $^ $(DESTDIR)/$(PREFIX)/bin

conntrack-flush: CFLAGS += `pkg-config libnetfilter_conntrack --cflags --libs`
conntrack-flush: nfct-flush-net.o

route-monitor: CFLAGS += `pkg-config libnl-1 --cflags --libs`
