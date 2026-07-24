---
title: Root Cause Catalog
description: '| RC-006 | **节点与 apiserver 网络不通** | 中 | D2.7, D2.2, D1.2 | `evt_api_unreachable`,
  `evt_policy_block`, `evt_route_fail` |'
summary: '| RC-006 | **节点与 apiserver 网络不通** | 中 | D2.7, D2.2, D1.2 | `evt_api_unreachable`,
  `evt_policy_block`, `evt_route_fail` |'
category: skills
tags:
- k8s
- troubleshooting
- skill
- apiserver
- kubelet
- containerd
tier: core
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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
| RC-009 | **内核问题/硬件异常** | 低 | D2.9 | `evt_kernel_panic`, `evt_driver_issue` |
| RC-010 | **NTP 时间不同步** | 低 | D2.10, D2.8 | `evt_time_skew_tls` |
| RC-011 | **CNI 插件异常** | 中 | D3.2, D1.2 | `evt_cni_fail` |
| RC-012 | **节点被手动 cordon/drain** | 低 | D1.4, D1.1 | `evt_cordon`（非问题） |

---

### 详细根因描述



## 相关链接

- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[技能/工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Pod 故障排查]]

## 根因分类体系

### 根因分类树

```
根因
├── 配置错误 (40%)
│   ├── YAML 语法错误
│   ├── 资源限制不当
│   └── 网络策略错误
├── 资源不足 (25%)
│   ├── CPU/内存不足
│   ├── 磁盘空间不足
│   └── 网络带宽不足
├── 软件缺陷 (20%)
│   ├── 应用 Bug
│   ├── 依赖服务故障
│   └── 版本不兼容
└── 基础设施 (15%)
    ├── 节点故障
    ├── 网络分区
    └── 存储故障
```

### 根因定位方法

| 方法 | 适用场景 | 工具 |
|---|---|---|
| 二分法 | 多组件系统 | 日志、监控 |
| 对比法 | 正常/异常对比 | diff、监控 |
| 排除法 | 多可能原因 | 逐项验证 |
| 重现法 | 间歇性问题 | 压测、混沌工程 |

## 面试要点

1. **Q：如何建立根因知识库？**
   A：故障复盘归档→分类统计→提取模式→编写 Runbook→定期更新。

2. **Q：最常见的 K8s 故障根因？**
   A：配置错误(40%)、资源不足(25%)、软件缺陷(20%)、基础设施(15%)。

3. **Q：如何加速根因定位？**
   A：完善监控、FTA 故障树、历史案例匹配、自动化工具、经验积累。

## Related

- [[技能/工作负载/pod/方法论/Symptom Vector Matching Engine.md|[[Symptom Vector Matching Engine|Symptom Vector Matching Engine]]]] — Symptom Vector Matching Engine
- [[实体/kubelet.md|kubelet]] — kubelet
- [[containerd]] — containerd
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
