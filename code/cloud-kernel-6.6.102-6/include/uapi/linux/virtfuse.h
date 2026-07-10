/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _LINUX_VIRTFUSE_H
#define _LINUX_VIRTFUSE_H

#include <linux/types.h>
#include <linux/ioctl.h>

/* Maximum number of devices supported. */
#define VIRT_FUSE_MAX_DEVICES		1024

/*
 * Clone a fuse device sharing the fuse connection bound to the specified
 * virtual device.
 */
#define VIRTFUSE_IOC_CLONE		_IO(0x99, 1)

/* Reset the specified virtual device */
#define VIRTFUSE_IOC_RESET		_IO(0x99, 2)

/* Print all mountinfo of the specified virtual device. */
#define VIRTFUSE_IOC_GET_MOUNTS		_IO(0x99, 3)

/*
 * @len	indicates the size of the buffer indicated by @buf
 * @buf	indicates a buffer to contain the output mountinfo of the specified
 * virtual device.
 */
struct virtfuse_mounts_buf {
	__u32	len;
	__u8	buf[];
};

#endif
