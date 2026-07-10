// SPDX-License-Identifier: GPL-2.0
/*
 * Provide a pstore frontend which can log all messages that are send
 * to tty drivers when there are some problems with drivers or there
 * is no access to serial ports.
 */

#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/console.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include "internal.h"

DEFINE_STATIC_KEY_FALSE(ttyprobe_key);

#define TTYPROBE_NAME "ttyprobe"
#undef pr_fmt
#define pr_fmt(fmt) TTYPROBE_NAME ": " fmt

static void do_write_ttymsg(const unsigned char *buf, int count,
			    struct pstore_info *psinfo)
{
	struct pstore_record record, newline;
	char *lbreak = "\n";

	pstore_record_init(&record, psinfo);
	record.type = PSTORE_TYPE_TTYPROBE;
	record.size = count;
	record.buf = (char *)buf;

	pstore_record_init(&newline, psinfo);
	newline.type = PSTORE_TYPE_TTYPROBE;
	newline.size = strlen(lbreak);
	newline.buf = lbreak;

	psinfo->write(&record);
	psinfo->write(&newline);
}

void pstore_register_ttyprobe(void)
{
	static_branch_enable(&ttyprobe_key);
}

void pstore_start_ttyprobe(const unsigned char *buf, int count)
{
	struct pstore_info_list *entry;

	rcu_read_lock();
	list_for_each_entry_rcu(entry, &psback->list_entry, list)
		if (entry->psi->flags & PSTORE_FLAGS_TTYPROBE)
			do_write_ttymsg(buf, count, entry->psi);
	rcu_read_unlock();
}

void pstore_unregister_ttyprobe(void)
{
	static_branch_disable(&ttyprobe_key);
}
