include $(top_srcdir)/scripts/subarch.include
ARCH ?= $(SUBARCH)

LIBLIVEUPDATE_SRCDIR := $(selfdir)/liveupdate/lib

LIBLIVEUPDATE_C := liveupdate.c

LIBLIVEUPDATE_OUTPUT := $(OUTPUT)/libliveupdate

LIBLIVEUPDATE_O := $(patsubst %.c, $(LIBLIVEUPDATE_OUTPUT)/%.o, $(LIBLIVEUPDATE_C))

LIBLIVEUPDATE_O_DIRS := $(shell dirname $(LIBLIVEUPDATE_O) | uniq)
$(shell mkdir -p $(LIBLIVEUPDATE_O_DIRS))

CFLAGS += -I$(LIBLIVEUPDATE_SRCDIR)/include

$(LIBLIVEUPDATE_O): $(LIBLIVEUPDATE_OUTPUT)/%.o : $(LIBLIVEUPDATE_SRCDIR)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c $< -o $@

EXTRA_CLEAN += $(LIBLIVEUPDATE_OUTPUT)
