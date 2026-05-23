---
title: Root Cause Catalog
description: '| RC-006 | **节点与 apiserver 网络不通** | 中 | D2.7, D2.2, D1.2 | `evt_api_unreachable`, `evt_policy_block`, `evt_route_fail`
  |'
category: skills
tags:
- k8s
- troubleshooting
- skill
- apiserver
- kubelet
- containerd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Root Cause Catalog 是什么
- 如何 Root Cause Catalog
trigger_keywords:
- Root
- Cause
- Catalog
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# Root Cause Catalog

### 根因总览表

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | **[[kubelet|kubelet]] 进程崩溃或未运行** | 高 | D2.1, D2.2, D1.5 | `evt_kubelet_down`, `evt_heartbeat_fail` |
| RC-002 | **容器运行时（containerd）异常** | 高 | D2.3, D2.4, D2.6, D1.2 | `evt_rt_down`, `evt_cri_sock`, `evt_rt_hang` |
| RC-003 | **节点磁盘空间耗尽（DiskPressure）** | 高 | D1.2, D2.5, D1.3 | `evt_disk_pressure`, `evt_image_gc_fail` |
| RC-004 | **节点内存耗尽（MemoryPressure）** | 中 | D1.2, D2.5, D2.9 | `evt_mem_pressure`, `evt_and_mem_low` |
| RC-005 | **节点 PID 耗尽（PIDPressure）** | 中 | D1.2, D2.5, D2.2 | `evt_pid_exhaust` |
| RC-006 | **节点与 apiserver 网络不通** | 中 | D2.7, D2.2, D1.2 | `evt_api_unreachable`, `evt_policy_block`, `evt_route_fail` |
| RC-007 | **kubelet 客户端证书过期** | 中 | D2.8, D2.2, D2.7 | `evt_kubelet_cert`, `evt_node_cert_expire` |
| RC-008 | **PLEG 不健康导致 NotReady** | 中 | D2.6, D1.2, D2.3 | `evt_pleg`, `evt_and_pleg_timeout`, `evt_and_pleg_overload` |
| RC-009 | **内核故障/硬件异常** | 低 | D2.9 | `evt_kernel_panic`, `evt_driver_issue` |
| RC-010 | **NTP 时间不同步** | 低 | D2.10, D2.8 | `evt_time_skew_tls` |
| RC-011 | **CNI 插件异常** | 中 | D3.2, D1.2 | `evt_cni_fail` |
| RC-012 | **节点被手动 cordon/drain** | 低 | D1.4, D1.1 | `evt_cordon`（非故障） |

---

### 详细根因描述



## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[skills/troubleshoot-pod-issues.md|Pod 故障排查]]

## Related

- [[skills/Symptom Vector Matching Engine.md|[[Symptom Vector Matching Engine|Symptom Vector Matching Engine]]]] — Symptom Vector Matching Engine
- [[entities/kubelet.md|kubelet]] — kubelet
- [[containerd]] — containerd
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
