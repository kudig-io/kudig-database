/* SPDX-License-Identifier: GPL-2.0*/
/*
 * Copyright (c) 2022 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */

#ifndef _NBL_DEV_H_
#define _NBL_DEV_H_

#include "nbl_core.h"
#include "nbl_export_rdma.h"
#include "nbl_dev_user.h"
#include "nbl_sysfs.h"

#define NBL_DEV_MGT_TO_COMMON(dev_mgt)		((dev_mgt)->common)
#define NBL_DEV_MGT_TO_DEV(dev_mgt)		NBL_COMMON_TO_DEV(NBL_DEV_MGT_TO_COMMON(dev_mgt))
#define NBL_DEV_MGT_TO_COMMON_DEV(dev_mgt)	((dev_mgt)->common_dev)
#define NBL_DEV_MGT_TO_CTRL_DEV(dev_mgt)	((dev_mgt)->ctrl_dev)
#define NBL_DEV_MGT_TO_NET_DEV(dev_mgt)		((dev_mgt)->net_dev)
#define NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt)	((dev_mgt)->rdma_dev)
#define NBL_DEV_MGT_TO_USER_DEV(dev_mgt)	((dev_mgt)->user_dev)
#define NBL_DEV_MGT_TO_REP_DEV(dev_mgt)		((dev_mgt)->rep_dev)
#define NBL_DEV_COMMON_TO_MSIX_INFO(dev_common)	(&(dev_common)->msix_info)
#define NBL_DEV_CTRL_TO_TASK_INFO(dev_ctrl)	(&(dev_ctrl)->task_info)
#define NBL_DEV_FACTORY_TO_TASK_INFO(dev_factory)	(&(dev_factory)->task_info)
#define NBL_DEV_MGT_TO_EMP_CONSOLE(dev_mgt)	((dev_mgt)->emp_console)
#define NBL_DEV_MGT_TO_NETDEV_OPS(dev_mgt)	((dev_mgt)->net_dev->ops)

#define NBL_DEV_MGT_TO_SERV_OPS_TBL(dev_mgt)	((dev_mgt)->serv_ops_tbl)
#define NBL_DEV_MGT_TO_SERV_OPS(dev_mgt)	(NBL_DEV_MGT_TO_SERV_OPS_TBL(dev_mgt)->ops)
#define NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt)	(NBL_DEV_MGT_TO_SERV_OPS_TBL(dev_mgt)->priv)
#define NBL_DEV_MGT_TO_RES_PT_OPS(adapter)	(&(NBL_DEV_MGT_TO_SERV_OPS_TBL(dev_mgt)->pt_ops))
#define NBL_DEV_MGT_TO_CHAN_OPS_TBL(dev_mgt)	((dev_mgt)->chan_ops_tbl)
#define NBL_DEV_MGT_TO_CHAN_OPS(dev_mgt)	(NBL_DEV_MGT_TO_CHAN_OPS_TBL(dev_mgt)->ops)
#define NBL_DEV_MGT_TO_CHAN_PRIV(dev_mgt)	(NBL_DEV_MGT_TO_CHAN_OPS_TBL(dev_mgt)->priv)

#define DEFAULT_MSG_ENABLE (NETIF_MSG_DRV | NETIF_MSG_PROBE | NETIF_MSG_LINK | \
			    NETIF_MSG_IFDOWN | NETIF_MSG_IFUP)

#define NBL_STRING_NAME_LEN			(32)
#define NBL_DEFAULT_MTU				(1500)

#define NBL_MAX_CARDS				16

#define NBL_KEEPALIVE_TIME_CYCLE		(10 * HZ)

#define NBL_DEV_BATCH_RESET_FUNC_NUM		(32)
#define NBL_DEV_BATCH_RESET_USEC		(1000000)

#define NBL_TIME_LEN				(32)
#define NBL_SAVED_TRACES_NUM			(16)

#define NBL_DEV_FW_RESET_WAIT_TIME		(3500)

enum nbl_reset_status {
	NBL_RESET_INIT,
	NBL_RESET_SEND,
	NBL_RESET_DONE,
	NBL_RESET_STATUS_MAX
};

struct nbl_task_info {
	struct nbl_adapter *adapter;
	struct nbl_dev_mgt *dev_mgt;
	struct work_struct offload_network_task;
	struct work_struct fw_hb_task;
	struct delayed_work fw_reset_task;
	struct work_struct clean_adminq_task;
	struct work_struct ipsec_task;
	struct work_struct adapt_desc_gother_task;
	struct work_struct clean_abnormal_irq_task;
	struct work_struct recovery_abnormal_task;
	struct work_struct report_temp_task;
	struct work_struct report_reboot_task;
	struct work_struct reset_task;
	enum nbl_reset_event reset_event;
	enum nbl_reset_status reset_status[NBL_MAX_FUNC];
	struct timer_list serv_timer;
	unsigned long serv_timer_period;

	bool fw_resetting;
	bool timer_setup;
};

struct nbl_reset_task_info {
	struct work_struct task;
	enum nbl_reset_event event;
};

enum nbl_msix_serv_type {
	/* virtio_dev has a config vector_id, and the vector_id need is 0 */
	NBL_MSIX_VIRTIO_TYPE = 0,
	NBL_MSIX_NET_TYPE,
	NBL_MSIX_MAILBOX_TYPE,
	NBL_MSIX_ABNORMAL_TYPE,
	NBL_MSIX_ADMINDQ_TYPE,
	NBL_MSIX_RDMA_TYPE,
	NBL_MSIX_TYPE_MAX

};

struct nbl_msix_serv_info {
	char irq_name[NBL_STRING_NAME_LEN];
	u16 num;
	u16 base_vector_id;
	/* true: hw report msix, hw need to mask actively */
	bool hw_self_mask_en;
};

struct nbl_msix_info {
	struct nbl_msix_serv_info serv_info[NBL_MSIX_TYPE_MAX];
	struct msix_entry *msix_entries;
};

struct nbl_dev_common {
	struct nbl_dev_mgt *dev_mgt;
	struct device *hwmon_dev;
	struct nbl_msix_info msix_info;
	char mailbox_name[NBL_STRING_NAME_LEN];
	// for ctrl-dev/net-dev mailbox recv msg
	struct work_struct clean_mbx_task;

	struct devlink_ops *devlink_ops;
	struct devlink *devlink;
	struct nbl_reset_task_info reset_task;
};

struct nbl_dev_factory {
	struct nbl_task_info task_info;
};

enum nbl_dev_temp_status {
	NBL_TEMP_STATUS_NORMAL = 0,
	NBL_TEMP_STATUS_WARNING,
	NBL_TEMP_STATUS_CRIT,
	NBL_TEMP_STATUS_EMERG,
	NBL_TEMP_STATUS_MAX
};

enum nbl_emp_log_level {
	NBL_EMP_ALERT_LOG_FATAL = 0,
	NBL_EMP_ALERT_LOG_ERROR = 1,
	NBL_EMP_ALERT_LOG_WARNING = 2,
	NBL_EMP_ALERT_LOG_INFO = 3,
};

struct nbl_fw_reporter_ctx {
	u64 timestamp;
	u32 temp_num;
	char reboot_report_time[NBL_TIME_LEN];
};

struct nbl_fw_temp_trace_data {
	u64 timestamp;
	u32 temp_num;
};

struct nbl_fw_reboot_trace_data {
	char local_time[NBL_TIME_LEN];
};

struct nbl_health_reporters {
	struct {
		struct nbl_fw_temp_trace_data trace_data[NBL_SAVED_TRACES_NUM];
		u8 saved_traces_index;
		struct mutex lock; /* protect reading data of temp_trace_data*/
	} temp_st_arr;

	struct {
		struct nbl_fw_reboot_trace_data trace_data[NBL_SAVED_TRACES_NUM];
		u8 saved_traces_index;
		struct mutex lock; /* protect reading data of reboot_trace_data*/
	} reboot_st_arr;

	struct nbl_fw_reporter_ctx reporter_ctx;
	struct devlink_health_reporter *fw_temp_reporter;
	struct devlink_health_reporter *fw_reboot_reporter;
};

struct nbl_dev_ctrl {
	struct nbl_task_info task_info;
	enum nbl_dev_temp_status temp_status;
	struct nbl_health_reporters health_reporters;
};

enum nbl_dev_emp_alert_event {
	NBL_EMP_EVENT_TEMP_ALERT = 1,
	NBL_EMP_EVENT_LOG_ALERT = 2,
	NBL_EMP_EVENT_MAX
};

enum nbl_dev_temp_threshold {
	NBL_TEMP_NOMAL_THRESHOLD = 85,
	NBL_TEMP_WARNING_THRESHOLD = 105,
	NBL_TEMP_CRIT_THRESHOLD = 115,
	NBL_TEMP_EMERG_THRESHOLD = 120,
};

struct nbl_dev_temp_alarm_info {
	int logvel;
#define NBL_TEMP_ALARM_STR_LEN		128
	char alarm_info[NBL_TEMP_ALARM_STR_LEN];
};

struct nbl_dev_vsi_controller {
	u16 queue_num;
	u16 queue_free_offset;
	void *vsi_list[NBL_VSI_MAX];
};

struct nbl_dev_net_ops {
	int (*setup_netdev_ops)(void *priv, struct net_device *netdev,
				struct nbl_init_param *param);
	int (*setup_ethtool_ops)(void *priv, struct net_device *netdev,
				 struct nbl_init_param *param);
	int (*setup_dcbnl_ops)(void *priv, struct net_device *netdev,
			       struct nbl_init_param *param);
};

struct nbl_dev_attr_info {
	struct nbl_netdev_name_attr dev_name_attr;
};

struct nbl_dev_net {
	struct net_device *netdev;
	struct nbl_dev_attr_info dev_attr;
	struct nbl_lag_member *lag_mem;
	struct nbl_dev_net_ops *ops;
	u8 lag_inited;
	u8 eth_id;
	struct nbl_dev_vsi_controller vsi_ctrl;
	u16 total_queue_num;
	u16 kernel_queue_num;
	u16 user_queue_num;
	u16 total_vfs;
	struct nbl_net_qos qos_config;
	struct nbl_net_mirror mirror_config;
};

struct nbl_dev_virtio {
	u8 device_msix;
};

struct nbl_dev_rdma_event_data {
	struct list_head node;
	/* Lag event will be processed async, so we need to fully store the param in case it is
	 * released by caller.
	 *
	 * callback_data will always be dev_mgt, which will not be released, so don't bother.
	 */
	struct nbl_event_param event_data;
	void *callback_data;
	u16 type;
};

struct nbl_dev_rdma {
	struct auxiliary_device *adev;
	struct auxiliary_device *grc_adev;
	struct auxiliary_device *bond_adev;

	struct work_struct abnormal_event_task;

	struct work_struct event_task;
	struct list_head event_param_list;
	struct mutex event_lock;		/* Protect event_param_list */

	int adev_index;
	u32 mem_type;
	bool has_rdma;
	bool has_grc;
	u16 func_id;
	u16 lag_id;
	bool bond_registered;
	bool bond_shaping_configed;

	bool is_halting;
	bool event_ready;
	bool mirror_enable;
	bool has_abnormal_event_task;
	atomic_t adev_busy;
};

struct nbl_dev_emp_console {
	struct nbl_dev_mgt *dev_mgt;
	unsigned int id;
	atomic_t opened;
	wait_queue_head_t wait;
	struct cdev cdev;
	struct kfifo rx_fifo;
	struct ktermios termios;
};

struct nbl_dev_user_iommu_group {
	struct mutex dma_tree_lock; /* lock dma tree */
	struct list_head group_next;
	struct kref     kref;
	struct rb_root dma_tree;
	struct iommu_group *iommu_group;
	struct device *dev;
	struct device *mdev;
	struct vfio_device *vdev;
};

struct nbl_dev_user {
	struct vfio_device *vdev;
	struct device mdev;
	struct notifier_block iommu_notifier;
	struct device *dev;
	struct nbl_adapter *adapter;
	struct nbl_dev_user_iommu_group *group;
	void *shm_msg_ring;
	u64 dma_limit;
	atomic_t open_cnt;
	int minor;
	int network_type;
	bool iommu_status;
	bool remap_status;
	bool user_promisc_mode;
	bool user_mcast_mode;
	u16 user_vsi;
};

struct nbl_vfio_device {
	struct vfio_device vdev;
	struct nbl_dev_user *user;
};

#define NBL_USERDEV_TO_VFIO_DEV(user)	((user)->vdev)
#define NBL_VFIO_DEV_TO_USERDEV(vdev)	(*(struct nbl_dev_user **)((vdev) + 1))
struct nbl_dev_rep {
	struct nbl_rep_data *rep;
	int num_vfs;
};

struct nbl_dev_mgt {
	struct nbl_common_info *common;
	struct nbl_service_ops_tbl *serv_ops_tbl;
	struct nbl_channel_ops_tbl *chan_ops_tbl;
	struct nbl_dev_common *common_dev;
	struct nbl_dev_ctrl *ctrl_dev;
	struct nbl_dev_net *net_dev;
	struct nbl_dev_rdma *rdma_dev;
	struct nbl_dev_emp_console *emp_console;
	struct nbl_dev_rep *rep_dev;
	struct nbl_dev_user *user_dev;
};

struct nbl_dev_vsi_feature {
	u16 has_lldp:1;
	u16 has_lacp:1;
	u16 rsv:14;
};

struct nbl_dev_vsi_ops {
	int (*register_vsi)(struct nbl_dev_mgt *dev_mgt, struct nbl_init_param *param,
			    void *vsi_data);
	int (*setup)(struct nbl_dev_mgt *dev_mgt, struct nbl_init_param *param,
		     void *vsi_data);
	void (*remove)(struct nbl_dev_mgt *dev_mgt, void *vsi_data);
	int (*start)(void *dev_priv, struct net_device *netdev, void *vsi_data);
	void (*stop)(void *dev_priv, void *vsi_data);
	int (*netdev_build)(struct nbl_dev_mgt *dev_mgt, struct nbl_init_param *param,
			    struct net_device *netdev, void *vsi_data);
	void (*netdev_destroy)(struct nbl_dev_mgt *dev_mgt, void *vsi_data);
};

struct nbl_dev_vsi {
	struct nbl_dev_vsi_ops *ops;
	struct net_device *netdev;
	struct net_device *napi_netdev;
	struct nbl_register_net_result register_result;
	struct nbl_dev_vsi_feature feature;
	u16 vsi_id;
	u16 queue_offset;
	u16 queue_num;
	u16 queue_size;
	u16 in_kernel;
	u8 index;
	bool enable;
	bool use_independ_irq;
	bool static_queue;
};

struct nbl_dev_vsi_tbl {
	struct nbl_dev_vsi_ops vsi_ops;
	bool vf_support;
	bool only_nic_support;
	u16 in_kernel;
	bool use_independ_irq;
	bool static_queue;
};

#define NBL_DEV_BOARD_ID_MAX			NBL_DRIVER_DEV_MAX
struct nbl_dev_board_id_entry {
	u32 board_key; /* domain << 16 | bus_id */
	u8 refcount;
	bool valid;
};

struct nbl_dev_board_id_table {
	struct nbl_dev_board_id_entry entry[NBL_DEV_BOARD_ID_MAX];
};

int nbl_dev_setup_rdma_dev(struct nbl_adapter *adapter, struct nbl_init_param *param);
void nbl_dev_remove_rdma_dev(struct nbl_adapter *adapter);
int nbl_dev_start_rdma_dev(struct nbl_adapter *adapter);
void nbl_dev_stop_rdma_dev(struct nbl_adapter *adapter);
int nbl_dev_resume_rdma_dev(struct nbl_adapter *adapter);
int nbl_dev_suspend_rdma_dev(struct nbl_adapter *adapter);
void nbl_dev_rdma_process_abnormal_event(struct nbl_dev_rdma *rdma_dev);
void nbl_dev_rdma_process_flr_event(struct nbl_dev_rdma *rdma_dev, u16 vsi_id);
size_t nbl_dev_rdma_qos_cfg_store(struct nbl_dev_mgt *dev_mgt, int offset,
				  const char *buf, size_t count);
size_t nbl_dev_rdma_qos_cfg_show(struct nbl_dev_mgt *dev_mgt, int offset, char *buf);

int nbl_dev_init_emp_console(struct nbl_adapter *adapter);
void nbl_dev_destroy_emp_console(struct nbl_adapter *adapter);
int nbl_dev_setup_hwmon(struct nbl_adapter *adapter);
void nbl_dev_remove_hwmon(struct nbl_adapter *adapter);
struct nbl_dev_vsi *nbl_dev_vsi_select(struct nbl_dev_mgt *dev_mgt, u8 vsi_index);

int nbl_netdev_add_sysfs(struct net_device *netdev, struct nbl_dev_net *net_dev);
int nbl_netdev_add_mirror_sysfs(struct net_device *netdev, struct nbl_dev_net *net_dev);
void nbl_netdev_remove_sysfs(struct nbl_dev_net *net_dev);
void nbl_netdev_remove_mirror_sysfs(struct nbl_dev_net *net_dev);
void nbl_net_add_name_attr(struct nbl_netdev_name_attr *dev_name_attr, char *rep_name);
void nbl_net_remove_dev_attr(struct nbl_dev_net *net_dev);
#endif
