/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */
/*
 * Userspace interface for HYGON Platform Security Processor (PSP)
 * commands.
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: Liyang Han <hanliyang@hygon.cn>
 */

#ifndef __PSP_HYGON_USER_H__
#define __PSP_HYGON_USER_H__

#include <linux/types.h>

/*****************************************************************************/
/***************************** CSV interface *********************************/
/*****************************************************************************/

/**
 * CSV guest/platform commands
 */
enum {
	CSV_PLATFORM_INIT = 101,
	CSV_PLATFORM_SHUTDOWN = 102,
	CSV_DOWNLOAD_FIRMWARE = 128,
	CSV_HGSC_CERT_IMPORT = 201,

	CSV_MAX,
};

/**
 * struct csv_user_data_hgsc_cert_import - HGSC_CERT_IMPORT command parameters
 *
 * @hgscsk_cert_address: HGSCSK certificate chain
 * @hgscsk_cert_len: length of HGSCSK certificate
 * @hgsc_cert_address: HGSC certificate chain
 * @hgsc_cert_len: length of HGSC certificate
 */
struct csv_user_data_hgsc_cert_import {
	__u64 hgscsk_cert_address;              /* In */
	__u32 hgscsk_cert_len;                  /* In */
	__u64 hgsc_cert_address;                /* In */
	__u32 hgsc_cert_len;                    /* In */
} __packed;

/**
 * struct csv_user_data_download_firmware - DOWNLOAD_FIRMWARE command parameters
 *
 * @address: physical address of CSV firmware image
 * @length: length of the CSV firmware image
 */
struct csv_user_data_download_firmware {
	__u64 address;				/* In */
	__u32 length;				/* In */
} __packed;

/* The CSV RTMR version in the kernel */
#define CSV_RTMR_VERSION_MAX	1U
#define CSV_RTMR_VERSION_MIN	1U

/* The size of CSV RTMR register. */
#define CSV_RTMR_REG_SIZE	32 /* SM3 */

/* The size of the CSV RTMR extend data. */
#define CSV_RTMR_EXTEND_LEN	CSV_RTMR_REG_SIZE

/**
 * The number of the CSV RTMR registers.
 *
 * For version 1, the number of RTMR registers for a guest is 5. The mapping
 * from TPM2 PCR index to RTMR index is shown as follows:
 *
 *   +---------------+--------------------------------------+---------+
 *   | TPM PCR Index | Event Log Measurement Register Index |  RTMR   |
 *   |---------------|--------------------------------------|-------- |
 *   |    0          |    0                                 | RTMR[0] |
 *   |    1,7        |    1                                 | RTMR[1] |
 *   |    2~6        |    2                                 | RTMR[2] |
 *   |    8~15       |    3                                 | RTMR[3] |
 *   |    16,23      |    4                                 | RTMR[4] |
 *   +---------------+--------------------------------------+---------+
 */
#define CSV_RTMR_REG_NUM	5

/* The maximum index of the CSV RTMR registers. */
#define CSV_RTMR_REG_INDEX_MAX	(CSV_RTMR_REG_NUM - 1)

#endif	/* __PSP_HYGON_USER_H__ */
