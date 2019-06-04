bin_PROGRAMS = serial-lcm-bridge simple-serial-lcm-bridge serial-lcm-source
dist_noinst_DATA = \
	README.md \
	LICENSE \
	lcmtypes/raw_bytes_t.lcm \
	doc/serial-lcm-bridge.1.ronn.md

EXTRA_DIST = build-aux/git-version-gen

AM_CFLAGS = -Wall -std=gnu99 -I@builddir@

raw_%.c raw_%.h: lcmtypes/raw_%.lcm
	$(LCMGEN) --c --c-hpath @builddir@ --c-cpath @builddir@ $^

BUILT_SOURCES = \
	raw_bytes_t.h \
	raw_bytes_t.c
serial_lcm_bridge_SOURCES = c/bridge.c
#noinst_HEADERS = c/r2/r2_epoch.h
nodist_serial_lcm_bridge_SOURCES = raw_bytes_t.c
serial_lcm_bridge_CFLAGS = $(LCM_CFLAGS) $(AM_CFLAGS)
serial_lcm_bridge_LDFLAGS = $(LCM_LIBS)

simple_serial_lcm_bridge_SOURCES = c/simple-bridge.c
nodist_simple_serial_lcm_bridge_SOURCES = raw_bytes_t.c
simple_serial_lcm_bridge_CFLAGS = $(LCM_CFLAGS) $(AM_CFLAGS)
simple_serial_lcm_bridge_LDFLAGS = $(LCM_LIBS)

serial_lcm_source_SOURCES = c/source.c
nodist_serial_lcm_source_SOURCES = raw_bytes_t.c
serial_lcm_source_CFLAGS = $(LCM_CFLAGS) $(AM_CFLAGS)
serial_lcm_source_LDFLAGS = $(LCM_LIBS)

TESTS = test-send_raw_bytes

check_PROGRAMS = test-send_raw_bytes

test_send_raw_bytes_SOURCES = test/c/send_raw_bytes.c
nodist_test_send_raw_bytes_SOURCES = raw_bytes_t.c
test_send_raw_bytes_CFLAGS = $(LCM_CFLAGS) $(AM_CFLAGS)
test_send_raw_bytes_LDFLAGS = $(LCM_LIBS)

MOSTLYCLEANFILES = $(BUILT_SOURCES)

if HAVE_RONN

man1_MANS = serial-lcm-bridge.man

serial-lcm-bridge.man: doc/serial-lcm-bridge.1.ronn.md
	$(RONN) --pipe -r $^ > @builddir@/$@

MOSTLYCLEANFILES += $(man1_MANS)

endif

