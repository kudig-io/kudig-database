/* SPDX-License-Identifier: GPL-2.0*/
/*
 * Copyright (c) 2022 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */

#ifndef _NBL_DEF_DISPATCH_H_
#define _NBL_DEF_DISPATCH_H_

#include "nbl_include.h"

#define NBL_DISP_OPS_TBL_TO_OPS(disp_ops_tbl)	((disp_ops_tbl)->ops)
#define NBL_DISP_OPS_TBL_TO_PRIV(disp_ops_tbl)	((disp_ops_tbl)->priv)

enum {
	NBL_DISP_CTRL_LVL_NEVER = 0,
	NBL_DISP_CTRL_LVL_MGT,
	NBL_DISP_CTRL_LVL_NET,
	NBL_DISP_CTRL_LVL_ALWAYS,
	NBL_DISP_CTRL_LVL_MAX,
};

struct nbl_dispatch_ops {
	int (*init_chip_module)(void *priv);
	void (*get_resource_pt_ops)(void *priv, struct nbl_resource_pt_ops *pt_ops);
	int (*queue_init)(void *priv);
	int (*vsi_init)(void *priv);
	int (*configure_msix_map)(void *priv, u16 num_net_msix, u16 num_others_msix,
				  bool net_msix_mask_en);
	int (*destroy_msix_map)(void *priv);
	int (*enable_mailbox_irq)(void *p, u16 vector_id, bool enable_msix);
	int (*enable_abnormal_irq)(void *p, u16 vector_id, bool enable_msix);
	int (*enable_adminq_irq)(void *p, u16 vector_id, bool enable_msix);
	u16 (*get_global_vector)(void *priv, u16 vsi_id, u16 local_vector_id);
	u16 (*get_msix_entry_id)(void *priv, u16 vsi_id, u16 local_vector_id);
	u32 (*get_chip_temperature)(void *priv, enum nbl_hwmon_type type, u32 senser_id);
	int (*get_module_temperature)(void *priv, u8 eth_id, enum nbl_hwmon_type type);

	int (*get_mbx_irq_num)(void *priv);
	int (*get_adminq_irq_num)(void *priv);
	int (*get_abnormal_irq_num)(void *priv);
	int (*alloc_rings)(void *priv, struct net_device *netdev, struct nbl_ring_param *param);
	void (*remove_rings)(void *priv);
	dma_addr_t (*start_tx_ring)(void *priv, u8 ring_index);
	void (*stop_tx_ring)(void *priv, u8 ring_index);
	dma_addr_t (*start_rx_ring)(void *priv, u8 ring_index, bool use_napi);
	void (*stop_rx_ring)(void *priv, u8 ring_index);
	void (*kick_rx_ring)(void *priv, u16 index);
	void (*set_rings_xdp_prog)(void *priv, void *prog);
	int (*register_xdp_rxq)(void *priv, u8 ring_index);
	void (*unregister_xdp_rxq)(void *priv, u8 ring_index);
	int (*dump_ring)(void *priv, struct seq_file *m, bool is_tx, int index);
	int (*dump_ring_stats)(void *priv, struct seq_file *m, bool is_tx, int index);
	struct nbl_napi_struct *(*get_vector_napi)(void *priv, u16 index);
	void (*set_vector_info)(void *priv, u8 *irq_enable_base, u32 irq_data,
				u16 index, bool mask_en);
	int (*register_net)(void *priv, struct nbl_register_net_param *register_param,
			    struct nbl_register_net_result *register_result);
	void (*register_vsi_ring)(void *priv, u16 vsi_index, u16 ring_offset, u16 ring_num);
	int (*unregister_net)(void *priv);
	int (*alloc_txrx_queues)(void *priv, u16 vsi_id, u16 queue_num);
	void (*free_txrx_queues)(void *priv, u16 vsi_id);
	int (*setup_queue)(void *priv, struct nbl_txrx_queue_param *param, bool is_tx);
	int (*remove_queue)(void *priv, struct nbl_txrx_queue_param *param, bool is_tx);
	void (*remove_all_queues)(void *priv, u16 vsi_id);
	int (*register_vsi2q)(void *priv, u16 vsi_index, u16 vsi_id,
			      u16 queue_offset, u16 queue_num);
	int (*setup_q2vsi)(void *priv, u16 vsi_id);
	void (*remove_q2vsi)(void *priv, u16 vsi_id);
	int (*setup_rss)(void *priv, u16 vsi_id);
	void (*remove_rss)(void *priv, u16 vsi_id);
	int (*cfg_dsch)(void *priv, u16 vsi_id, bool vld);
	int (*setup_cqs)(void *priv, u16 vsi_id, u16 real_qps, bool rss_indir_set);
	void (*remove_cqs)(void *priv, u16 vsi_id);
	int (*cfg_qdisc_mqprio)(void *priv, struct nbl_tc_qidsc_param *param);
	void (*clear_queues)(void *priv, u16 vsi_id);
	int (*check_offload_status)(void *priv, bool *is_down);
	u16 (*get_vsi_global_qid)(void *priv, u16 vsi_id, u16 local_qid);
	u16 (*get_local_queue_id)(void *priv, u16 vsi_id, u16 global_queue_id);
	u16 (*get_vsi_global_queue_id)(void *priv, u16 vsi_id, u16 local_qid);

	u8* (*get_msix_irq_enable_info)(void *priv, u16 global_vector_id, u32 *irq_data);
	int (*set_spoof_check_addr)(void *priv, u16 vsi_id, u8 *mac);
	int (*set_vf_spoof_check)(void *priv, u16 vsi_id, int vfid, u8 enable);
	void (*get_base_mac_addr)(void *priv, u8 *mac);

	int (*add_macvlan)(void *priv, u8 *mac, u16 vlan, u16 vsi);
	void (*del_macvlan)(void *priv, u8 *mac, u16 vlan, u16 vsi);
	int (*add_lag_flow)(void *priv, u16 vsi);
	void (*del_lag_flow)(void *priv, u16 vsi);
	int (*add_lldp_flow)(void *priv, u16 vsi);
	void (*del_lldp_flow)(void *priv, u16 vsi);
	int (*add_multi_rule)(void *priv, u16 vsi);
	void (*del_multi_rule)(void *priv, u16 vsi);
	int (*cfg_multi_mcast)(void *priv, u16 vsi, u16 enable);
	int (*setup_multi_group)(void *priv);
	void (*remove_multi_group)(void *priv);
	void (*clear_accel_flow)(void *priv, u16 vsi_id);
	void (*clear_flow)(void *priv, u16 vsi_id);
	void (*dump_flow)(void *priv, struct seq_file *m);

	u16 (*get_vsi_id)(void *priv, u16 func_id, u16 type);
	void (*get_eth_id)(void *priv, u16 vsi_id, u8 *eth_mode, u8 *eth_id, u8 *logic_eth_id);
	int (*set_promisc_mode)(void *priv, u16 vsi_id, u16 mode);
	int (*set_mtu)(void *priv, u16 vsi_id, u16 mtu);
	int (*get_max_mtu)(void *priv);
	u32 (*get_tx_headroom)(void *priv);
	void (*get_rep_feature)(void *priv, struct nbl_register_net_result *register_result);
	void (*get_rep_queue_info)(void *priv, u16 *queue_num, u16 *queue_size);
	void (*get_user_queue_info)(void *priv, u16 *queue_num, u16 *queue_size, u16 vsi_id);
	void (*set_eswitch_mode)(void *priv, u16 switch_mode);
	u16 (*get_eswitch_mode)(void *priv);
	int (*alloc_rep_data)(void *priv, int num_vfs, u16 vf_base_vsi_id);
	void (*free_rep_data)(void *priv);
	void (*set_rep_netdev_info)(void *priv, void *rep_data);
	void (*unset_rep_netdev_info)(void *priv);
	struct net_device *(*get_rep_netdev_info)(void *priv, u16 rep_data_index);
	int (*disable_phy_flow)(void *priv, u8 eth_id);
	int (*enable_phy_flow)(void *priv, u8 eth_id);
	void (*init_acl)(void *priv);
	void (*uninit_acl)(void *priv);
	int (*set_upcall_rule)(void *priv, u8 eth_id, u16 vsi_id);
	int (*unset_upcall_rule)(void *priv, u8 eth_id);
	void (*set_shaping_dport_vld)(void *priv, u8 eth_id, bool vld);
	void (*set_dport_fc_th_vld)(void *priv, u8 eth_id, bool vld);
	void (*get_rep_stats)(void *priv, u16 rep_vsi_id,
			      struct nbl_rep_stats *rep_stats, bool is_tx);
	u16 (*get_rep_index)(void *priv, u16 vsi_id);

	void (*get_firmware_version)(void *priv, char *firmware_verion, u8 max_len);
	int (*get_driver_info)(void *priv, struct nbl_driver_info *driver_info);
	void (*get_queue_stats)(void *priv, u8 queue_id,
				struct nbl_queue_stats *queue_stats, bool is_tx);
	int (*get_queue_err_stats)(void *priv, u8 queue_id,
				   struct nbl_queue_err_stats *queue_err_stats, bool is_tx);
	int (*get_eth_mac_stats)(void *priv, u32 eth_id,
				 struct nbl_eth_mac_stats *eth_mac_stats, u32 data_len);
	int (*get_rmon_stats)(void *priv, u32 eth_id,
			      struct nbl_rmon_stats *rmon_stats, u32 data_len);
	void (*get_net_stats)(void *priv, struct nbl_stats *queue_stats);
	int (*get_eth_ctrl_stats)(void *priv, u32 eth_id,
				  struct nbl_eth_ctrl_stats *eth_ctrl_stats, u32 data_len);
	int (*get_pause_stats)(void *priv, u32 eth_id,
			       struct nbl_pause_stats *pause_stats, u32 data_len);
	void (*get_private_stat_len)(void *priv, u32 *len);
	void (*get_private_stat_data)(void *priv, u32 eth_id, u64 *data, u32 data_len);
	void (*fill_private_stat_strings)(void *priv, u8 *strings);
	int (*get_eth_abnormal_stats)(void *priv, u8 eth_id,
				      struct nbl_eth_abnormal_stats *eth_abnormal_stats);
	u16 (*get_max_desc_num)(void *priv);
	u16 (*get_min_desc_num)(void *priv);
	u16 (*get_tx_desc_num)(void *priv, u32 ring_index);
	u16 (*get_rx_desc_num)(void *priv, u32 ring_index);
	void (*set_tx_desc_num)(void *priv, u32 ring_index, u16 desc_num);
	void (*set_rx_desc_num)(void *priv, u32 ring_index, u16 desc_num);
	void (*get_coalesce)(void *priv, u16 vector_id, struct nbl_chan_param_get_coalesce *ec);
	void (*set_coalesce)(void *priv, u16 vector_id, u16 num_net_msix, u16 pnum, u16 rate);
	u16 (*get_intr_suppress_level)(void *priv, u64 rate, u16 last_level);
	void (*set_intr_suppress_level)(void *priv, u16 vector_id,
					u16 num_net_msix, u16 level);
	void (*get_rxfh_indir_size)(void *priv, u16 vsi_id, u32 *rxfh_indir_size);
	void (*get_rxfh_indir)(void *priv, u16 vsi_id, u32 *indir, u32 indir_size);
	int (*set_rxfh_indir)(void *priv, u16 vsi_id, const u32 *indir, u32 indir_size);
	void (*get_rxfh_rss_key_size)(void *priv, u32 *rxfh_rss_key_size);
	void (*get_rxfh_rss_key)(void *priv, u8 *rss_key, u32 rss_key_size);
	void (*get_rxfh_rss_alg_sel)(void *priv, u16 vsi_id, u8 *alg_sel);
	int (*set_rxfh_rss_alg_sel)(void *priv, u16 vsi_id, u8 alg_sel);
	int (*get_port_attributes)(void *priv);
	int (*enable_port)(void *priv, bool enable);
	void (*init_port)(void *priv);
	int (*cfg_eth_bond_info)(void *priv, struct nbl_lag_member_list_param *param);
	int (*get_eth_bond_info)(void *priv, struct nbl_bond_param *param);
	void (*recv_port_notify)(void *priv);
	int (*get_port_state)(void *priv, u8 eth_id, struct nbl_port_state *port_state);
	int (*get_fec_stats)(void *priv, u8 eth_id, struct nbl_fec_stats *fec_stats);
	int (*set_port_advertising)(void *priv, struct nbl_port_advertising *port_advertising);
	int (*get_module_info)(void *priv, u8 eth_id, struct ethtool_modinfo *info);
	int (*get_module_eeprom)(void *priv, u8 eth_id, struct ethtool_eeprom *eeprom, u8 *data);
	int (*get_link_state)(void *priv, u8 eth_id, struct nbl_eth_link_info *eth_link_info);
	int (*get_link_down_count)(void *priv, u8 eth_id, u64 *link_down_count);
	int (*get_link_status_opcode)(void *priv, u8 eth_id, u32 *link_status_opcode);
	int (*set_eth_mac_addr)(void *priv, u8 *mac, u8 eth_id);
	int (*process_abnormal_event)(void *priv, struct nbl_abnormal_event_info *abnomal_info);
	int (*ctrl_port_led)(void *priv, u8 eth_id, enum nbl_led_reg_ctrl led_ctrl, u32 *led_reg);
	int (*nway_reset)(void *priv, u8 eth_id);
	int (*set_wol)(void *priv, u8 eth_id, bool enable);
	void (*adapt_desc_gother)(void *priv);
	void (*set_desc_high_throughput)(void *priv);
	void (*flr_clear_net)(void *priv, u16 vfid);
	void (*flr_clear_queues)(void *priv, u16 vfid);
	void (*flr_clear_accel_flow)(void *priv, u16 vfid);
	void (*flr_clear_flows)(void *priv, u16 vfid);
	void (*flr_clear_interrupt)(void *priv, u16 vfid);
	void (*flr_clear_accel)(void *priv, u16 vfid);
	void (*flr_clear_rdma)(void *priv, u16 vfid);
	u16 (*covert_vfid_to_vsi_id)(void *priv, u16 vfid);
	void (*unmask_all_interrupts)(void *priv);
	void (*keep_alive)(void *priv);
	void (*cfg_eth_bond_event)(void *priv, bool enable);
	int (*set_bridge_mode)(void *priv, u16 bmode);
	void (*cfg_txrx_vlan)(void *priv, u16 vlan_tci, u16 vlan_proto, u8 vsi_index);

	void (*setup_rdma_id)(void *priv);
	void (*remove_rdma_id)(void *priv);
	void (*register_rdma)(void *priv, u16 vsi_id, struct nbl_rdma_register_param *param);
	void (*unregister_rdma)(void *priv, u16 vsi_id);
	void (*register_rdma_bond)(void *priv, struct nbl_lag_member_list_param *list_param,
				   struct nbl_rdma_register_param *register_param);
	void (*unregister_rdma_bond)(void *priv, u16 lag_id);
	u8 __iomem * (*get_hw_addr)(void *priv, size_t *size);
	u64 (*get_real_hw_addr)(void *priv, u16 vsi_id);
	u16 (*get_function_id)(void *priv, u16 vsi_id);
	void (*get_real_bdf)(void *priv, u16 vsi_id, u8 *bus, u8 *dev, u8 *function);
	int (*enable_lag_protocol)(void *priv, u16 eth_id, bool lag_en);
	int (*cfg_lag_hash_algorithm)(void *priv, u16 eth_id, u16 lag_id,
				      enum netdev_lag_hash hash_type);
	int (*cfg_lag_member_fwd)(void *priv, u16 eth_id, u16 lag_id, u8 fwd);
	int (*cfg_lag_member_list)(void *priv, struct nbl_lag_member_list_param *param);
	int (*cfg_lag_member_up_attr)(void *priv, u16 eth_id, u16 lag_id, bool enable);
	int (*cfg_duppkt_info)(void *priv, struct nbl_lag_member_list_param *param);
	int (*cfg_duppkt_mcc)(void *priv, struct nbl_lag_member_list_param *param);
	int (*cfg_bond_shaping)(void *priv, u8 eth_id, bool enable);
	void (*cfg_bgid_back_pressure)(void *priv, u8 main_eth_id, u8 other_eth_id, bool enable);

	bool (*check_fw_heartbeat)(void *priv);
	bool (*check_fw_reset)(void *priv);
	int (*flash_lock)(void *priv);
	int (*flash_unlock)(void *priv);
	int (*flash_prepare)(void *priv);
	int (*flash_image)(void *priv, u32 module, const u8 *data, size_t len);
	int (*flash_activate)(void *priv);
	void (*get_phy_caps)(void *priv, u8 eth_id, struct nbl_phy_caps *phy_caps);
	int (*set_sfp_state)(void *priv, u8 eth_id, u8 state);
	int (*set_eth_loopback)(void *priv, u8 enable);
	struct sk_buff *(*clean_rx_lb_test)(void *priv, u32 ring_index);
	int (*passthrough_fw_cmd)(void *priv, struct nbl_passthrough_fw_cmd_param *param,
				  struct nbl_passthrough_fw_cmd_param *result);
	int (*update_ring_num)(void *priv);
	int (*update_rdma_cap)(void *priv);
	int (*update_rdma_mem_type)(void *priv);
	u16 (*get_rdma_cap_num)(void *priv);
	int (*set_ring_num)(void *priv, struct nbl_fw_cmd_net_ring_num_param *param);

	u32 (*check_active_vf)(void *priv);
	int (*get_board_id)(void *priv);

	void (*get_reg_dump)(void *priv, u32 *data, u32 len);
	int (*get_reg_dump_len)(void *priv);

	u32 (*get_adminq_tx_buf_size)(void *priv);
	int (*emp_console_write)(void *priv, char *buf, size_t count);
	bool (*get_product_flex_cap)(void *priv, enum nbl_flex_cap_type cap_type);
	bool (*get_product_fix_cap)(void *priv, enum nbl_fix_cap_type cap_type);
	int (*alloc_ktls_tx_index)(void *priv, u16 vsi);
	void (*free_ktls_tx_index)(void *priv, u32 index);
	void (*cfg_ktls_tx_keymat)(void *priv, u32 index, u8 mode, u8 *salt, u8 *key, u8 key_len);
	int (*alloc_ktls_rx_index)(void *priv, u16 vsi);
	void (*free_ktls_rx_index)(void *priv, u32 index);
	void (*cfg_ktls_rx_keymat)(void *priv, u32 index, u8 mode, u8 *salt, u8 *key, u8 key_len);
	void (*cfg_ktls_rx_record)(void *priv, u32 index, u32 tcp_sn, u64 rec_num, bool init);
	int (*add_ktls_rx_flow)(void *priv, u32 index, u32 *data, u16 vsi);
	void (*del_ktls_rx_flow)(void *priv, u32 index);

	int (*alloc_ipsec_tx_index)(void *priv, struct nbl_ipsec_cfg_info *cfg_info);
	void (*free_ipsec_tx_index)(void *priv, u32 index);
	int (*alloc_ipsec_rx_index)(void *priv, struct nbl_ipsec_cfg_info *cfg_info);
	void (*free_ipsec_rx_index)(void *priv, u32 index);
	void (*cfg_ipsec_tx_sad)(void *priv, u32 index, struct nbl_ipsec_sa_entry *sa_entry);
	void (*cfg_ipsec_rx_sad)(void *priv, u32 index, struct nbl_ipsec_sa_entry *sa_entry);
	int (*add_ipsec_tx_flow)(void *priv, u32 index, u32 *data, u16 vsi);
	void (*del_ipsec_tx_flow)(void *priv, u32 index);
	int (*add_ipsec_rx_flow)(void *priv, u32 index, u32 *data, u16 vsi);
	void (*del_ipsec_rx_flow)(void *priv, u32 index);
	bool (*check_ipsec_status)(void *priv);
	u32 (*get_dipsec_lft_info)(void *priv);
	void (*handle_dipsec_soft_expire)(void *priv, u32 index);
	void (*handle_dipsec_hard_expire)(void *priv, u32 index);
	u32 (*get_uipsec_lft_info)(void *priv);
	void (*handle_uipsec_soft_expire)(void *priv, u32 index);
	void (*handle_uipsec_hard_expire)(void *priv, u32 index);
	void (*get_board_info)(void *priv, struct nbl_board_port_info *board_info);

	void (*dummy_func)(void *priv);

	void (*configure_virtio_dev_msix)(void *priv, u16 vector);
	void (*configure_rdma_msix_off)(void *priv, u16 vector);
	void (*configure_virtio_dev_ready)(void *priv);

	int (*switchdev_init_cmdq)(void *priv);
	int (*switchdev_deinit_cmdq)(void *priv);
	int (*add_tc_flow)(void *priv, struct nbl_tc_flow_param *param);
	int (*del_tc_flow)(void *priv, struct nbl_tc_flow_param *param);
	int (*flow_index_lookup)(void *priv, struct nbl_flow_index_key key);

	bool (*tc_tun_encap_lookup)(void *priv, struct nbl_rule_action *rule_act,
				    struct nbl_tc_flow_param *param);
	int (*tc_tun_encap_del)(void *priv, struct nbl_encap_key *key);
	int (*tc_tun_encap_add)(void *priv, struct nbl_rule_action *action);

	int (*set_tc_flow_info)(void *priv);
	int (*unset_tc_flow_info)(void *priv);
	int (*get_tc_flow_info)(void *priv);
	int (*query_tc_stats)(void *priv, struct nbl_stats_param *param);

	u32 (*get_p4_version)(void *priv);
	int (*get_p4_info)(void *priv, char *verify_code);
	int (*load_p4)(void *priv, struct nbl_load_p4_param *param);
	int (*load_p4_default)(void *priv);
	int (*get_p4_used)(void *priv);
	int (*set_p4_used)(void *priv, int p4_type);
	u16 (*get_vf_base_vsi_id)(void *priv, u16 pf_id);

	int (*add_nd_upcall_flow)(void *priv, u16 vsi_id, bool for_pmd);
	void (*del_nd_upcall_flow)(void *priv);

	dma_addr_t (*restore_abnormal_ring)(void *priv, int ring_index, int type);
	int (*restart_abnormal_ring)(void *priv, int ring_index, int type);
	int (*restore_hw_queue)(void *priv, u16 vsi_id, u16 local_queue_id,
				dma_addr_t dma, int type);
	int (*stop_abnormal_sw_queue)(void *priv, u16 local_queue_id, int type);
	int (*stop_abnormal_hw_queue)(void *priv, u16 vsi_id, u16 local_queue_id, int type);
	u16 (*get_vf_function_id)(void *priv, u16 vsi_id, int vf_id);
	u16 (*get_vf_vsi_id)(void *priv, u16 vsi_id, int vf_id);
	bool (*check_vf_is_active)(void *priv, u16 func_id);
	int (*check_vf_is_vdpa)(void *priv, u16 func_id, u8 *is_vdpa);
	int (*get_vdpa_vf_stats)(void *priv, u16 func_id, struct nbl_vf_stats *vf_stats);
	int (*get_uvn_pkt_drop_stats)(void *priv, u16 vsi_id,
				      u16 num_queues, u32 *uvn_stat_pkt_drop);
	int (*get_ustore_pkt_drop_stats)(void *priv);
	int (*get_ustore_total_pkt_drop_stats)(void *priv, u8 eth_id,
					       struct nbl_ustore_stats *ustore_stats);
	int (*set_pmd_debug)(void *priv, bool pmd_debug);

	void (*register_func_mac)(void *priv, u8 *mac, u16 func_id);
	int (*register_func_trust)(void *priv, u16 func_id, bool trusted,
				   bool *should_notify);
	int (*register_func_vlan)(void *priv, u16 func_id, u16 vlan_tci,
				  u16 vlan_proto, bool *should_notify);
	int (*register_func_rate)(void *priv, u16 func_id, int rate);
	int (*register_func_link_forced)(void *priv, u16 func_id, u8 link_forced,
					 bool *should_notify);
	int (*get_link_forced)(void *priv, u16 vsi_id);
	int (*set_tx_rate)(void *priv, u16 func_id, int tx_rate, int burst);
	int (*set_rx_rate)(void *priv, u16 func_id, int rx_rate, int burst);

	void (*get_driver_version)(void *priv, char *ver, int len);

	void (*register_dev_name)(void *priv, u16 vsi_id, char *name);
	void (*get_dev_name)(void *priv, u16 vsi_id, char *name);

	int (*get_mirror_table_id)(void *priv, u16 vsi_id, int dir,
				   bool mirror_en, u8 *mt_id);
	int (*configure_mirror)(void *priv, u16 func_id, bool mirror_en, int dir,
				u8 mt_id);
	int (*configure_mirror_table)(void *priv, bool mirror_en, u16 func_id, u8 mt_id);
	int (*clear_mirror_cfg)(void *priv, u16 func_id);

	int (*get_fd_flow)(void *priv, u16 vsi_id, u32 location,
			   enum nbl_chan_fdir_rule_type rule_type,
			   struct nbl_chan_param_fdir_replace *cmd);
	int (*get_fd_flow_cnt)(void *priv, enum nbl_chan_fdir_rule_type rule_type, u16 vsi_id);
	int (*config_fd_flow_state)(void *priv, enum nbl_chan_fdir_rule_type rule_type,
				    u16 vsi_id, u16 state);
	int (*get_fd_flow_all)(void *priv, struct nbl_chan_param_get_fd_flow_all *param,
			       u32 *rule_locs);
	int (*get_fd_flow_max)(void *priv);

	int (*replace_fd_flow)(void *priv, struct nbl_chan_param_fdir_replace *info);
	int (*remove_fd_flow)(void *priv, enum nbl_chan_fdir_rule_type rule_type,
			      u32 loc, u16 vsi_id);

	void (*cfg_fd_update_event)(void *priv, bool enable);
	void (*dump_fd_flow)(void *priv, struct seq_file *m);

	void (*get_xdp_queue_info)(void *priv, u16 *queue_num, u16 *queue_size, u16 vsi_id);
	void (*set_hw_status)(void *priv, enum nbl_hw_status hw_status);
	void (*get_active_func_bitmaps)(void *priv, unsigned long *bitmap, int max_func);
	int (*configure_qos)(void *priv, u8 eth_id, u8 *pfc, u8 trust, u8 *dscp2prio_map);
	int (*configure_rdma_bw)(void *priv, u8 eth_id, int rdma_bw);
	int (*get_pfc_buffer_size)(void *priv, u8 eth_id, u8 prio, int *xoff, int *xon);
	int (*set_pfc_buffer_size)(void *priv, u8 eth_id, u8 prio, int xoff, int xon);
	int (*set_rate_limit)(void *priv, enum nbl_traffic_type type, u32 rate);
	int (*set_tc_wgt)(void *priv, u16 vsi_id, u8 *weight, u8 num_tc);

	u32 (*get_perf_dump_length)(void *priv);
	u32 (*get_perf_dump_data)(void *priv, u8 *buffer, u32 size);
	void (*cfg_mirror_outputport_event)(void *priv, bool enable);
	int (*check_flow_table_spec)(void *priv, u16 vlan_cnt, u16 unicast_cnt, u16 multicast_cnt);
	u32 (*get_dvn_desc_req)(void *priv);
	void (*set_dvn_desc_req)(void *priv, u32 desc_req);
};

struct nbl_dispatch_ops_tbl {
	struct nbl_dispatch_ops *ops;
	void *priv;
};

int nbl_disp_init(void *p, struct nbl_init_param *param);
void nbl_disp_remove(void *p);

#endif
