/* SPDX-License-Identifier: GPL-2.0 */
/**
 * Copyright (C), 2020, Linkdata Technologies Co., Ltd.
 *
 * @file: sxe_upgrade.h
 * @author: Linkdata
 * @date: 2025.02.16
 * @brief:
 * @note:
 */

#ifndef __SXE_UPGRADE_H__
#define __SXE_UPGRADE_H__

#include "sxe.h"
#include "sxe_netdev.h"
#include "sxe_log.h"

#define SXE_INVAL_U16 (0xFFFF)
#define SXE_MAX_UPDATE_FWTYPE (8)
#define SXE_UPGRADE_PKGS_NAME_LEN (256)
#define SXE_FRAG_LEN (2048)
#define SXE_FW_VENDOR_LEN (8U)
#define SXE_FW_SIGN_LEN (64U)
#define SXE_FW_PKEY_LEN (68U)
#define SXE_BIT_MAP_64 (64)

#define SXE_PAD_1_K (1024U)
#define SXE_PACK_DATA_BEGIN_NUM (0x327f68cd)
#define SXE_DATABEGIN_NUM (0x327f68ab)

#define SXE_UPGRADE_PROTOCAL_VERSION (0x00000001)
#define SXE_FRAG_ENABLE (1)
#define SXE_ETH_UPGRADE_DEV_TYPE_CTRL (1)
#define SXE_UPGRADE_FW_TYPE (36)
#define SXE_MSG_MAGIC_CODE (0x12345678)

#define SXE_FWHEADER_IMAGETYPE(fw_header) ((fw_header)->image_type_append)
#define SXE_SET_BIT64(x, y) ((x) |= ((u64)1 << (y)))

#define SXE_CMD_LIMIT_OFFSET (24)
#define SXE_CMD_OBJECT_OFFSET (16)
#define SXE_CMD_TYPE_OFFSET (8)
#define SXE_CMD_CODE_OFFSET (0)

#define SXE_MK_LIMIT(limit, object, type, code) \
	({ \
		(((limit) & 0xff) << SXE_CMD_LIMIT_OFFSET) | \
		(((object) & 0xff) << SXE_CMD_OBJECT_OFFSET) | \
		(((type) & 0xff) << SXE_CMD_TYPE_OFFSET) | \
		(((code) & 0xff) << SXE_CMD_CODE_OFFSET); \
	})

#define SXE_CMD_LIMIT_RAID (0b0001)
#define SXE_CMD_LIMIT_HBA (0b0010)
#define SXE_CMD_LIMIT_SWITCH (0b0100)
#define SXE_CMD_NO_LIMIT \
	((s32)SXE_CMD_LIMIT_RAID | SXE_CMD_LIMIT_HBA | SXE_CMD_LIMIT_SWITCH)

#define SXE_CMD_CHANNEL_IOCTL (0b00100000)
#define SXE_CMD_UPDATE (0xb)
#define SXE_CMD_DOWNLOAD (0xa)

#define SXE_CMD_FW_DOWNLOAD_PREPARE \
		SXE_MK_LIMIT(SXE_CMD_NO_LIMIT | SXE_CMD_CHANNEL_IOCTL, \
			    SXE_CMD_UPDATE, SXE_CMD_DOWNLOAD, 2)
#define SXE_CMD_FW_DOWNLOAD_OPEN \
		SXE_MK_LIMIT(SXE_CMD_NO_LIMIT | SXE_CMD_CHANNEL_IOCTL, \
			    SXE_CMD_UPDATE, SXE_CMD_DOWNLOAD, 3)
#define SXE_CMD_FW_DOWNLOAD_FLASH \
		SXE_MK_LIMIT(SXE_CMD_NO_LIMIT | SXE_CMD_CHANNEL_IOCTL, \
			    SXE_CMD_UPDATE, SXE_CMD_DOWNLOAD, 4)
#define SXE_CMD_FW_DOWNLOAD_CLOSE \
		SXE_MK_LIMIT(SXE_CMD_NO_LIMIT | SXE_CMD_CHANNEL_IOCTL, \
			    SXE_CMD_UPDATE, SXE_CMD_DOWNLOAD, 5)
#define SXE_CMD_FW_DOWNLOAD_END \
		SXE_MK_LIMIT(SXE_CMD_NO_LIMIT | SXE_CMD_CHANNEL_IOCTL, \
			    SXE_CMD_UPDATE, SXE_CMD_DOWNLOAD, 6)

struct sxe_pd_position {
	u8 enclid;
	u8 pad;
	u16 slotid;
};

struct sxe_phy_position {
	u64 enclsasaddr;
	u64 physasaddr;
	u8 enclid;
	u8 phyid;
	u8 pad[6];
};

struct sxe_mgl_id_group {
	u8 type;
	u8 pad[7];
	union {
		u16 deviceid;
		struct sxe_pd_position pdposition;
		u16 vdid;
		u16 dgid;
		struct sxe_phy_position phyposition;
		u16 laneid;
	};
};

struct sxe_mgr_msg_info {
	u32 magic;
	u32 opcode;
	u32 error;
	u32 timeout;
	u32 starttime;
	u32 runver;
	u32 length;
	u32 ackoffset;
	u32 acklength;
	u32 leftmsgcount;
	u32 msgindex;
	u8  reserved[4];
	u64 uuid;
	u8 servtype;
	struct {
		u8 ack : 4;
		u8 tlv : 4;
	} index;
	u8 funcid : 1;
	u8 pad : 7;
	u8 reserved2[5];
	struct sxe_mgl_id_group id;
	u64 traceid;
#ifdef HAVE_REPLACE_ZERO_ARRAY_WITH_FLEXIBLE
	u8 body[];
#else
	u8 body[0];
#endif
};

struct sxe_update_flash_param {
	u64 uuid;
	u32 frag_index;
	u32 pack_len;
	u32 frag_num;
	u32 fw_type;
	u8 *pack_data;
	u8 *raw_data;
};

struct sxe_upgd_image_info {
	__le32 offset;
	__le32 image_len;
	__le32 fw_type;
};

struct sxe_upgrade_fw_array {
	__le32 fw_cnt;
	struct sxe_upgd_image_info fw_arr[SXE_MAX_UPDATE_FWTYPE];
};

struct sxe_pkg_header {
	__le32 magic;
	__le32 fw_count;
	__le32 pack_time;
	__le32 pack_len;
	__le32 pkg_check_sum;
	__le32 pkg_version;
	s8 pkg_name[SXE_UPGRADE_PKGS_NAME_LEN];
	u8 reserved[4];
};

struct sxe_region_header {
	__le32 magic;
	u8 vendor[SXE_FW_VENDOR_LEN];
	__le32 timestamp;
	__le32 image_len;
	__le32 image_type_append;
	u8 signature[SXE_FW_SIGN_LEN];
	u8 publickey[SXE_FW_PKEY_LEN];
	__le32 check_sum_file;
	__le32 image_type;
	__le32 image_format;
	__le32 entry_point;
	__le32 load_addr;
	__le32 reserved2;
	__le32 image_version;
	u8 reserved[68];
	__le32 check_sum_header;
};

struct sxe_upgrade_prepare_cmd {
	__le64 uuid;
	__le32 fw_type_cnt;
	u8 pad[4];
	__le64 fw_type_bitmap;
	bool is_pkg;
	u8 pad2[4];
	struct sxe_pkg_header pkg_hdr_info;
	u8 pad3[4];
};

struct sxe_upgrade_open_cmd {
	__le32 dev_type;
	__le32 fw_type;
	u8 pad1[SXE_PAD_1_K];
	__le32 pad2[2];
	u64 uuid;
	__le32 frag_num;
	__le32 frag_len;
	__le32 fw_len;
	__le32 no_sign_chk : 1;
	__le32 no_ver_chk : 1;
	__le32 force : 1;
	__le32 all : 1;
	__le32 backup : 1;
	__le32 is_fw_head : 1;
	__le32 no_reset : 1;
	__le32 forcehcb : 1;
	__le32 resetnow : 1;
	__le32 forceclose: 1;
	__le32 ispacket : 1;
	__le32 reserved : 21;
};

struct sxe_frag_head {
	__le64 uuid;
	__le32 version;
	__le32 frag_sid;
	__le32 frag_len;
	__le32 checksum;
	__le32 symbol_enable : 1;
	__le32 symbol_more : 1;
	__le32 symbol_reserve : 6;
	__le32 reserved : 24;
	u8 pad[4];
};

struct sxe_upgrade_flash_cmd {
	struct sxe_frag_head frag_head;
	u8 raw_data[SXE_FRAG_LEN];
};

struct sxe_upgrade_close_cmd {
	__le64 uuid;
	__le32 err_code;
	__le32 reset_now : 1;
	__le32 reserved : 31;
};

struct sxe_upgrade_end_cmd {
	__le64 uuid;
	__le32 err_code;
	__le32 fw_type;
};

s32 sxe_flash_package_from_file(struct net_device *dev, const char *filename);

#endif
