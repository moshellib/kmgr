
CC = gcc
CFLAGS = -W -O2 -g -DFRAMER_ALLOW_HANDLERS=1 -DFIX_ENDIAN=0
KMDOBJS = obj/main.o obj/sock.o obj/framer.o obj/seeprom.o obj/log.o obj/cmnutil.o obj/kio.o obj/kmgr.o obj/wdr85100.o obj/entry.o obj/prog.o obj/s19.o obj/serial.o obj/seriallinux.o obj/bcm85100_api.o obj/endian.o obj/hostCommLayer.o obj/hostDriver.o obj/utils.o obj/cmnstruct_api.o 
MANAGEMENT_TEST_SRCS = $(filter-out main.c kmgr.c,$(wildcard *.c loader/*.c brcm/*.c))

all: kmgr

.PHONY: all clean install deb test-loader test-core test-brcm test-protocol test-serial test-management test

test: test-loader test-core test-brcm test-protocol test-serial test-management

test-management:
	mkdir -p obj
	$(CC) $(CFLAGS) -ffunction-sections -fdata-sections -I. tests/management_test.c $(MANAGEMENT_TEST_SRCS) -Wl,--gc-sections -Wl,--wrap=kmgr_issue_command -lpthread -o obj/management_test
	./obj/management_test

test-serial:
	mkdir -p obj
	$(CC) $(CFLAGS) -I. tests/serial_test.c loader/serial.c loader/seriallinux.c -lutil -o obj/serial_test
	./obj/serial_test

test-protocol:
	mkdir -p obj
	$(CC) $(CFLAGS) -ffunction-sections -fdata-sections -I. tests/protocol_test.c loader/prog.c loader/s19.c -Wl,--gc-sections -o obj/protocol_test
	./obj/protocol_test

test-brcm:
	mkdir -p obj
	$(CC) $(CFLAGS) -ffunction-sections -fdata-sections -I. tests/brcm_test.c brcm/hostCommLayer.c brcm/hostDriver.c -Wl,--gc-sections -lpthread -o obj/brcm_test
	./obj/brcm_test

test-core:
	mkdir -p obj
	$(CC) $(CFLAGS) -ffunction-sections -fdata-sections -I. tests/core_test.c sock.c cmnutil.c framer.c kio.c seeprom.c -Wl,--gc-sections -lpthread -o obj/core_test
	./obj/core_test

test-loader:
	mkdir -p obj
	$(CC) $(CFLAGS) -I. tests/loader_entry_test.c loader/entry.c -o obj/loader_entry_test
	./obj/loader_entry_test

install: kmgr
	install -D -m 0755 kmgr $(DESTDIR)/usr/bin/kmgr
	install -d $(DESTDIR)/etc/kmgr $(DESTDIR)/usr/local/bin $(DESTDIR)/usr/lib/systemd/system $(DESTDIR)/usr/share/kmgr/kfs
	install -m 0644 resources/*.cfg $(DESTDIR)/etc/kmgr/
	install -m 0755 resources/*.sh $(DESTDIR)/usr/local/bin/
	install -m 0644 resources/*.service $(DESTDIR)/usr/lib/systemd/system/
	install -m 0644 kfs/z_skm.hex $(DESTDIR)/usr/share/kmgr/kfs/

deb:
	chmod +x debian/rules
	dpkg-buildpackage -us -uc -b

kmgr: $(KMDOBJS)
	gcc $(KMDOBJS) -lrt -lpthread -o kmgr

obj/ksh.o: ksh.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c ksh.c -o obj/ksh.o

obj/cmnutil.o: cmnutil.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c cmnutil.c -o obj/cmnutil.o

obj/main.o: main.c
	mkdir -p obj
	gcc $(CFLAGS) -c main.c -o obj/main.o

obj/sock.o: sock.c
	mkdir -p obj
	gcc $(CFLAGS) -c sock.c -o obj/sock.o

obj/kio.o: kio.c
	mkdir -p obj
	gcc $(CFLAGS) -c kio.c -o obj/kio.o

obj/kmgr.o: kmgr.c
	mkdir -p obj
	gcc $(CFLAGS) -c kmgr.c -o obj/kmgr.o

obj/framer.o: framer.c
	mkdir -p obj
	gcc $(CFLAGS) -c framer.c -o obj/framer.o

obj/cmnstruct_api.o: cmnstruct_api.c
	mkdir -p obj
	gcc $(CFLAGS) -c cmnstruct_api.c -o obj/cmnstruct_api.o

obj/seeprom.o: seeprom.c
	mkdir -p obj
	gcc $(CFLAGS) -c seeprom.c -o obj/seeprom.o

obj/log.o: log.c
	mkdir -p obj
	gcc $(CFLAGS) -c log.c -o obj/log.o

obj/wdr85100.o: wdr85100.c wdr85100.h
	mkdir -p obj
	gcc $(CFLAGS) -c wdr85100.c -o obj/wdr85100.o

obj/entry.o: loader/entry.c loader/hc08sprg.h loader/sysdeplinux.h loader/serial.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c loader/entry.c -I. -o obj/entry.o

obj/prog.o: loader/prog.c loader/hc08sprg.h loader/sysdeplinux.h loader/serial.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c loader/prog.c -o obj/prog.o

obj/s19.o: loader/s19.c loader/hc08sprg.h loader/sysdeplinux.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c loader/s19.c -o obj/s19.o

obj/serial.o: loader/serial.c loader/sysdeplinux.h loader/serial.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c loader/serial.c -o obj/serial.o

obj/seriallinux.o: loader/seriallinux.c loader/sysdeplinux.h loader/serial.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c loader/seriallinux.c -o obj/seriallinux.o

obj/bcm85100_api.o: brcm/bcm85100_api.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c brcm/bcm85100_api.c -o obj/bcm85100_api.o

obj/endian.o: brcm/endian.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c brcm/endian.c -o obj/endian.o

obj/hostCommLayer.o: brcm/hostCommLayer.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c brcm/hostCommLayer.c -o obj/hostCommLayer.o

obj/hostDriver.o: brcm/hostDriver.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c brcm/hostDriver.c -o obj/hostDriver.o

obj/utils.o: brcm/utils.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c brcm/utils.c -o obj/utils.o

clean:
	rm -rf obj kmgr

cu0:
	xterm -e cu -l /dev/ttyUSB0 -s 115200 &

cu1:
	xterm -e cu -l /dev/ttyUSB1 -s 115200 &
