/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *
 * Userspace interface for CSV guest driver
 *
 * Copyright (C) Hygon Info Technologies Ltd.
 */

#ifndef __VIRT_CSVGUEST_H__
#define __VIRT_CSVGUEST_H__

#include <linux/ioctl.h>
#include <linux/types.h>
#include <uapi/linux/psp-hygon.h>

/* Length of the user input datas used in VMMCALL */
#define CSV_REPORT_USER_DATA_LEN	64
#define CSV_REPORT_MNONCE_LEN		16
#define CSV_REPORT_HASH_LEN		32
#define CSV_REPORT_INPUT_DATA_LEN	(CSV_REPORT_USER_DATA_LEN + CSV_REPORT_MNONCE_LEN \
					+ CSV_REPORT_HASH_LEN)

/**
 * struct csv_guest_user_data_attestation - ATTESTATION command parameters.
 *	This is used for legacy attestation.
 *
 *	In the legacy attestation, the size of tee info portion of the
 *	attestation report is 0x150. The tee info does not contain rtmr.
 *
 * @user_data: user defined data, it's suggest that contains challenge data
 *	       from the relying party.
 * @mnonce: user's random nonce, just for anti-replay attack protection.
 * @hash: the sm3 hash of the @user_data and @mnonce.
 */
struct csv_guest_user_data_attestation {
	__u8 user_data[CSV_REPORT_USER_DATA_LEN];	/* In */
	__u8 mnonce[CSV_REPORT_MNONCE_LEN];		/* In */
	__u8 hash[CSV_REPORT_HASH_LEN];			/* In */
} __packed;

#define CSV_ATTESTATION_USER_DATA_EXT_LEN	132U

#define CSV_ATTESTATION_MAGIC_LEN		16
#define CSV_ATTESTATION_MAGIC_STRING		"ATTESTATION_EXT"

#define CSV_ATTESTATION_FLAG_REPORT_EXT_BIT	0
#define CSV_ATTESTATION_FLAG_REPORT_EXT		(1U << CSV_ATTESTATION_FLAG_REPORT_EXT)

/**
 * struct csv_guest_user_data_attestation_ext - ATTESTATION command parameters.
 *	This is used for extended attestation.
 *
 *	In the extended attestation, the size of tee info portion of the
 *	attestation report is 0x490. The tee info contains rtmr by default.
 *
 *	Currently, the extended attestation only supported for CSV3 guest.
 *
 * @user_data: user defined data, it's suggest that contains challenge data
 *	       from the relying party.
 * @mnonce: user's random nonce, just for anti-replay attack protection.
 * @hash: the sm3 hash of the @user_data and @mnonce.
 * @magic: The magic string indicates this is an extended attestation aware
 *	   request. Due to historical reasons, users always provide a buffer
 *	   that is much larger than the size of structure
 *	   csv_guest_user_data_attestation_ext. The magic string will be used to
 *	   determine whether the user space uses the extended attestation aware
 *	   request. If the user space is using such a request, the flags field
 *	   will be used to indicate the format of the attestation report.
 * @flags: the bit flags used to indicate how to extend the attestation report.
 */
struct csv_guest_user_data_attestation_ext {
	__u8  user_data[CSV_REPORT_USER_DATA_LEN];	/* In */
	__u8  mnonce[CSV_REPORT_MNONCE_LEN];		/* In */
	__u8  hash[CSV_REPORT_HASH_LEN];		/* In */
	__u8  magic[CSV_ATTESTATION_MAGIC_LEN];		/* In */
	__u32 flags;					/* In */
} __packed;

/**
 * struct csv_report_req - Request struct for CSV_CMD_GET_REPORT IOCTL.
 *
 * @report_data:User buffer with REPORT_DATA to be included into CSV_REPORT, and it's also
 *		user buffer to store CSV_REPORT output from VMMCALL[KVM_HC_VM_ATTESTATION].
 * @len:	Length of the user buffer.
 */
struct csv_report_req {
	u8 *report_data;
	int len;
};

/*
 * CSV_CMD_GET_REPORT - Get CSV_REPORT using VMMCALL[KVM_HC_VM_ATTESTATION]
 *
 * Return 0 on success, -EIO on VMMCALL execution failure, and
 * standard errno on other general error cases.
 */
#define CSV_CMD_GET_REPORT	_IOWR('D', 1, struct csv_report_req)

enum csv_guest_user_rtmr_subcmd {
	CSV_GUEST_USER_RTMR_STATUS	= 0x1,
	CSV_GUEST_USER_RTMR_START	= 0x2,
	CSV_GUEST_USER_RTMR_READ	= 0x3,
	CSV_GUEST_USER_RTMR_EXTEND	= 0x4,
};

/**
 * struct csv_guest_user_rtmr_status - RTMR_STATUS subcommand parameters
 *
 * @version: the RTMR version used in the guest. When in state
 *	     RTMR_STATE_INIT, it will be fixed.
 * @state: the state of the guest's RTMR.
 */
struct csv_guest_user_rtmr_status {
	__u16 version;	/* Out */
	__u8  state;	/* Out */
} __packed;

/**
 * struct csv_guest_user_rtmr_start - RTMR_START subcommand parameters
 *
 * @version: the RTMR version requested by the guest.
 */
struct csv_guest_user_rtmr_start {
	__u16 version;	/* In,Out */
} __packed;

/**
 * struct csv_guest_user_rtmr_read - RTMR_READ subcommand parameters
 *
 * @bitmap: the bitmap specified the RTMR registers to read.
 * @data: the buffer to store RTMR registers' data returned by the firmware.
 *	  If read more than one RTMR register, user should provide a larger
 *	  buffer.
 */
struct csv_guest_user_rtmr_read {
	__u32 bitmap;
	__u8  data[CSV_RTMR_REG_SIZE];
} __packed;

/**
 * struct csv_guest_user_rtmr_extend - RTMR_EXTEND subcommand parameters
 *
 * @index: the index for extending the RTMR register.
 * @rsvd: reserved, just for alignment.
 * @data_len: the length of the data to be extended.
 * @data: the data to be extended to the RTMR register.
 */
struct csv_guest_user_rtmr_extend {
	__u8  index;
	__u8  rsvd;
	__u16 data_len;
	__u8  data[CSV_RTMR_EXTEND_LEN];
} __packed;

/**
 * struct csv_rtmr_req - RTMR request command
 *
 * @buf: the RTMR subcommand buffer.
 * @len: the length of RTMR subcommand.
 * @subcmd_id: the identifier of the RTMR subcommand.
 * @rsvd: reserved, just for alignment.
 * @fw_error_code: the return code that a RTMR subcommand returned by the
 *		   firmware.
 */
struct csv_rtmr_req {
	__u64 buf;		/* In */
	__u64 len;		/* In,Out */
	__u16 subcmd_id;	/* In */
	__u16 rsvd;		/* Reserved */
	__u32 fw_error_code;	/* Out */
} __packed;

/*
 * CSV_CMD_RTMR - RTMR request with user data using secure call.
 *
 * Returns 0 on success, and standard errono on other failures.
 */
#define CSV_CMD_RTMR	_IOWR('D', 2, struct csv_rtmr_req)

#endif /* __VIRT_CSVGUEST_H__ */
