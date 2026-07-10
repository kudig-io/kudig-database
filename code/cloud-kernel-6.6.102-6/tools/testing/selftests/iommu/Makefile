# SPDX-License-Identifier: GPL-2.0-only
CFLAGS += -Wall -O2 -Wno-unused-function
CFLAGS += $(KHDR_INCLUDES)
LDLIBS += -lcap

CFLAGS += -D_GNU_SOURCE

TEST_GEN_PROGS :=
TEST_GEN_PROGS += iommufd
TEST_GEN_PROGS += iommufd_fail_nth

TEST_GEN_PROGS_EXTENDED += iommufd_liveupdate_kexec_test

include ../lib.mk
include ../liveupdate/lib/libliveupdate.mk

CFLAGS += -I$(top_srcdir)/tools/include
CFLAGS += -MD
CFLAGS += $(EXTRA_CFLAGS)

$(TEST_GEN_PROGS_EXTENDED): %: %.o $(LIBLIVEUPDATE_O)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH) $< $(LIBLIVEUPDATE_O) $(LDLIBS) -o $@

EXTRA_CLEAN += $(LIBLIVEUPDATE_O)
