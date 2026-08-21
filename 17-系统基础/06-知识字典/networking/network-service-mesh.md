---
title: Network Service Mesh
description: Network Service Mesh（NSM）是 CNCF Sandbox 项目，使用服务网格的概念来管理网络服务（L2/L3 VPN、防火墙、负载均衡等）...
summary: Network Service Mesh（NSM）是 CNCF Sandbox 项目，使用服务网格的概念来管理网络服务（L2/L3 VPN、防火墙、负载均衡等）...
category: dictionary
tags:
- k8s
- glossary
- networking
- multi-cluster
- cncf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Network Service Mesh 是什么
- NSM 详解
trigger_keywords:
- Network Service Mesh
- NSM
- dictionary
prerequisites:
- kubernetes
---



# Network Service Mesh（NSM）

## 概述

Network Service Mesh（NSM）是 CNCF Sandbox 项目，使用服务网格的概念来管理网络服务（L2/L3 VPN、防火墙、负载均衡等），将网络功能从硬件解耦到软件定义。

## 核心概念/原理

- **网络服务网格**：将网络功能软件化，按需编排
- **L2/L3 VPN**：跨集群的 L2/L3 网络连接
- **CNCF Sandbox**：活跃的 NFV/SDN 社区
- **与 K8s 集成**：基于 K8s CRD 管理网络服务

## 关键机制或特性

- NetworkService / NetworkServiceEndpoint CRD
- NSMGR（Network Service Mesh Registry）
- Forwarder 数据面（VPP/memif/Kernel）
- 多集群 L2/L3 VPN
- 与 Multus CNI 集成
- 支持 Intel VPP 高性能转发

## 使用场景与最佳实践

- 5G/Telco 的网络功能虚拟化
- 跨集群 L2/L3 VPN 连接
- 传统网络设备的软件化替代
- 多租户网络隔离
- 云原生 NFV 基础设施

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────────┐
│                     Kubernetes 集群                       │
│                                                         │
│  ┌──────────────┐   ┌──────────────────────────────┐   │
│  │  NSM 控制面   │   │       数据面 Forwarder        │   │
│  │              │   │                              │   │
│  │  Registry    │──▶│  VPP Forwarder (Intel VPP)   │   │
│  │  (CRD 存储)  │   │  Kernel Forwarder (XDP)      │   │
│  │              │   │  memif Forwarder (用户态)     │   │
│  │  NSMGR       │──▶│                              │   │
│  │  (网络服务    │   │  ┌─────┐  ┌─────┐  ┌─────┐  │   │
│  │   管理器)     │   │  │ Pod │  │ Pod │  │ Pod │  │   │
│  └──────────────┘   │  └─────┘  └─────┘  └─────┘  │   │
│         │           └──────────────────────────────┘   │
│         ▼                                               │
│  ┌──────────────┐   多集群 VPN 隧道 (WireGuard/VXLAN)   │
│  │ NSE (网络服务 │◀──────────────────────────▶ 远端集群   │
│  │  端点)        │                                      │
│  └──────────────┘                                      │
└─────────────────────────────────────────────────────────┘
```

### 源码关键路径（networkservicemesh/networkservicemesh）

| 模块 | 路径 | 职责 |
|------|------|------|
| NSMGR | `controlplane/pkg/nsmgr` | 网络服务管理器，处理连接请求与生命周期 |
| Registry | `controlplane/pkg/registry` | 服务注册与发现（内存/etcd 后端） |
| Forwarder | `forwarder/vpp/` | VPP 数据面转发实现 |
| SDK | `sdk/` | 供业务 Pod 与 Sidecar 使用的客户端库 |
| API | `api/` | NetworkService / NetworkServiceEndpoint CRD 定义 |

### 连接建立流程

1. 业务 Pod 通过 NSM SDK 或 Sidecar 发起 `Request` 连接请求
2. NSMGR 查询 Registry 找到匹配的 NetworkService 与 NSE
3. NSMGR 协调 Forwarder 在 Pod 与 NSE 之间建立数据面路径
4. Forwarder 配置接口（VPP 创建 memif 接口 / Kernel 创建 veth）
5. 连接状态上报 Registry，支持多集群跨域连接（通过 WireGuard 隧道封装）

## 生产案例

### 案例 1：5G 边缘集群跨域 L2 连接中断

| 时间 | 事件 |
|------|------|
| 10:12 | 5G 边缘集群与中心集群 L2 VPN 中断，业务 Pod 无法访问中心数据库 |
| 10:15 | NSMGR 日志显示 `request timeout`，NSE 心跳超时 |
| 10:20 | 检查 Forwarder 日志，发现 VPP 进程 OOM 重启 |
| 10:25 | 确认 VPP 数据面接口未重建，memif 连接悬挂 |
| 10:40 | 重启 Forwarder DaemonSet 并扩容内存限制，连接自动恢复 |

**根因**：VPP Forwarder 内存限制过小（默认 512Mi），在高吞吐场景 OOM 后接口重建逻辑存在竞态，导致 memif 悬挂。

**修复命令**：
```bash
# 查看 Forwarder 状态 🟢 只读
kubectl -n nsm get pods -l app=nsm-forwarder
# 扩容 VPP 内存并重启 🟡 中风险
kubectl -n nsm patch daemonset forwarder-vpp -p '{"spec":{"template":{"spec":{"containers":[{"name":"forwarder","resources":{"limits":{"memory":"2Gi"}}}]}}}}'
kubectl -n nsm rollout restart daemonset forwarder-vpp
# 验证连接恢复 🟢 只读
kubectl -n nsm logs -l app=nsm-forwarder --tail=50 | grep -i memif
```

### 案例 2：多集群 NSM 注册表数据不一致

**现象**：新增的 NSE 在部分集群可见、部分集群不可见，跨集群服务发现不稳定。

**诊断**：
- 对比各集群 Registry 中 NSE 列表，发现 etcd 同步延迟
- 检查 Registry 日志，出现 `etcd watch event lost` 告警
- 确认多集群场景下使用独立 etcd 实例而非共享实例

**修复**：统一 Registry 后端为共享 etcd（或引入 NSM 官方的 etcd 集群部署），并配置 watch 重连策略；定期执行 `nsmctl registry list` 校验一致性。

## 对比评测

| 维度 | NSM | Submariner | 手工 WireGuard 隧道 |
|------|-----|------------|---------------------|
| 抽象层级 | 连接级网络服务编排 | 集群级互联（Service/Cluster 发现） | 无抽象 |
| 数据面 | VPP/Kernel/memif 多选 | IPsec/WireGuard 隧道 | WireGuard 自管 |
| 服务发现 | NSM Registry 注册 | ServiceExport 声明 | 无 |
| 适用场景 | 5G/Telco NFV、NaaS | 多集群 Service 互通 | 临时连接 |
| 运维复杂度 | 高（组件多） | 中 | 低 |

**选型建议**：若需求是"跨集群访问 Service"，Submariner 更简单直接；若需求是"网络功能（防火墙/VPN/LB）作为服务按需提供"，NSM 才是正解。

**性能参考**：VPP Forwarder 在 DPDK 加速下可达 10Mpps+ 单核转发能力；Kernel Forwarder 依赖内核协议栈，典型吞吐 1-3Gbps（取决于主机网卡与 CPU）；memif 在共享内存场景下延迟可低至亚微秒级。生产选型时应以实际压测（`iperf3` + `netperf`）结果为准，避免凭文档指标做决策。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| 连接超时 | `nsmctl connection list` / `kubectl logs -l app=nsmgr` | NSE 未注册或 Forwarder 异常 |
| memif 悬挂 | `kubectl logs -l app=forwarder-vpp` | VPP OOM 后接口未重建 |
| 跨集群不通 | `nsmctl registry list --cluster remote` | Registry 未共享或隧道失效 |
| Pod 无网络 | `kubectl describe pod` 查 annotations | NSM SDK 注入失败或 Multus 冲突 |

**预防建议**：为 Forwarder 配置内存监控告警（使用率 >80% 触发）；所有集群统一 Registry 后端版本；升级前在预发集群先行验证多集群互通用例。

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 数据面 Forwarder OOM 或 memif 悬挂导致业务中断 | 立即扩容内存并重启 Forwarder，升级至修复版本 |
| P1 | 多集群注册表不一致、跨集群连接不稳定 | 规划 Registry 后端迁移至共享 etcd，验证 watch 语义 |
| P2 | 使用 VPP 转发但无性能收益（小流量场景） | 评估切换 Kernel Forwarder 降低运维复杂度 |

## 生产部署清单

- [ ] NSM 核心组件（registry/manager）HA 部署且版本一致
- [ ] NSC/NSE 的 selector 匹配关系已规划（避免跨命名空间误连）
- [ ] 网络服务（如 Memif/VLAN）的 endpoint 生命周期监控已配置
- [ ] 与现有 CNI 的协同方式已验证（NSM 通常承载特殊网络能力）
- [ ] 故障演练：NSE 实例重启 → NSC 重连与恢复验证

## 面试要点

> 以下 Q&A 覆盖 NSM 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：NSM 与普通 CNI（如 Calico）的核心区别是什么？**
   A：CNI 解决 Pod 网络连通问题（分配 IP、打通网络）；NSM 解决网络服务的按需编排问题（VPN、防火墙、负载均衡等网络功能的软件化与动态接入），支持连接粒度的网络服务组合与多集群互通，是"网络即服务"（NaaS）范式。

2. **Q：NSM 的三种 Forwarder（VPP/Kernel/memif）各自适用什么场景？**
   A：VPP 面向高性能 NFV 场景（5G/Telco），支持 SR-IOV 与硬件加速；Kernel Forwarder 基于内核协议栈，运维简单、适合通用场景；memif 是共享内存接口，用于进程间高性能转发，通常与 VPP 配合作为 Pod 接入点。

3. **Q：NSM 如何实现跨集群 L2 连接？**
   A：每个集群部署 NSMGR 与 Forwarder，多集群间通过 WireGuard/VXLAN 建立隧道；NSE 注册到共享 Registry（etcd），NSMGR 根据 NetworkService 选择跨集群路径，实现透明 L2 互通。

## 参考链接

- https://networkservicemesh.io/
- https://github.com/networkservicemesh/networkservicemesh

## Related

- [[17-系统基础/06-知识字典/networking/submariner.md|Submariner]]
- [[17-系统基础/06-知识字典/networking/cni.md|CNI]]
- [[17-系统基础/06-知识字典/networking/loxilb.md|LoxiLB]]
