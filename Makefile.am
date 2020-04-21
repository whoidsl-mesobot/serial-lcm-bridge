bin_PROGRAMS = serial-lcm-bridge simple-serial-lcm-bridge
dist_noinst_DATA = \
	README.md \
	LICENSE \
	lcmtypes/raw_bytes_t.lcm \
	doc/serial-lcm-bridge.1.ronn.md

EXTRA_DIST = .build-aux/git-version-gen

AM_CFLAGS = -std=gnu99 \
	-I@builddir@ \
	$(LCM_CFLAGS)

raw_%.c raw_%.h: lcmtypes/raw_%.lcm
	$(LCMGEN) --c --c-hpath @builddir@ --c-cpath @builddir@ $^

BUILT_SOURCES = \
	raw_bytes_t.h \
	raw_bytes_t.c

serial_lcm_bridge_SOURCES = c/bridges.h \
	c/r2_epoch.h \
	c/r2_sio.h \
	c/complex.h \
	c/complex.c
nodist_serial_lcm_bridge_SOURCES = raw_bytes_t.h raw_bytes_t.c
serial_lcm_bridge_CFLAGS = $(AM_CFLAGS)

simple_serial_lcm_bridge_SOURCES = c/bridges.h \
	c/r2_epoch.h \
	c/r2_sfd.h \
	c/simple.h \
	c/simple.c
nodist_simple_serial_lcm_bridge_SOURCES = raw_bytes_t.h raw_bytes_t.c
simple_serial_lcm_bridge_CFLAGS = $(AM_CFLAGS)

TESTS = test-send_raw_bytes

check_PROGRAMS = test-send_raw_bytes

test_send_raw_bytes_SOURCES = test/c/send_raw_bytes.c
nodist_test_send_raw_bytes_SOURCES = raw_bytes_t.h raw_bytes_t.c
test_send_raw_bytes_CFLAGS = $(AM_CFLAGS)

MOSTLYCLEANFILES = $(BUILT_SOURCES) *.gz *.bz2 *.xz

if HAVE_RONN

man1_MANS = serial-lcm-bridge.man

serial-lcm-bridge.man: doc/serial-lcm-bridge.1.ronn.md
	$(RONN) --pipe -r $^ > @builddir@/$@

MOSTLYCLEANFILES += $(man1_MANS)

endif

