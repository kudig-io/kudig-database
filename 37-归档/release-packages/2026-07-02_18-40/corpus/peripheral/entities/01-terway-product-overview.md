---
title: Terway 产品概览
description: '## 概述'
summary: 'Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- cilium
- flannel
- networkpolicy
- crd
- ebpf
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 产品概览 是什么
- 如何 Terway 产品概览
trigger_keywords:
- Terway
- 产品概览
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Terway 产品概览

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

title: 01 - Terway 产品概览 (Product Overview)

## 技术细节

### 3. 网络模式总览

Terway 提供五种网络模式，按性能和容量密度递增排列：

| 模式 | Pod IP 来源 | 网络接口 | 性能 (相对物理机) | 容量密度 | 内核要求 | 适用场景 |
|:---|:---|:---|:---:|:---:|:---|:---|
| **VPC** | VPC 路由表条目 | veth pair + Node 网络栈 | ~70% | 低 (受路由条目 48 条限制) | 无特殊要求 | 小规模集群、兼容性优先、已有 Flannel 迁移过渡 |
| **ENI** | 独占 ENI 主 IP | ENI 直通 | ~95% | 低 (受 ENI 配额限制) | 无特殊要求 | 核心数据库、网关、高性能隔离需求 |
| **ENIIP** | ENI 辅助 IP (Secondary IP) | veth pair + ENI | ~90% | 高 (推荐默认

### 5. 核心依赖

Terway 深度依赖以下阿里云基础设施和服务：

| 依赖 | 服务 | 说明 | 必需性 |
|:---|:---|:---|:---:|
| **VPC (专有网络)** | 阿里云 VPC | Pod 网络的底层承载平面，vSwitch 为 Pod 分配 VPC 内网 IP | 必需 |
| **ENI (弹性网卡)** | 阿里云 ECS ENI | ENI/ENIIP/IPVlan 模式的网络接口载体，每个 Pod 通过 ENI 接入 VPC | ENI 模式必需 |
| **OpenAPI** | 阿里云 ECS API | ENI 创建/删除/绑定/解绑，辅助 IP 分配/释放等操作 | 必需 |
| **RAM 角色** | 阿里云 RAM | Terway 通过 ECS 实例角色 (Instance RAM Role) 获取访问云资源的临时凭证 | 必需 |
| **安



## 与 K8s 网络模型的关系

Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]

## 生产部署建议

- 建议在生产环境中使用 ENI 多 IP 模式以提高 IP 利用率 ^[inferred]
- 密切监控 ENI 资源使用情况，避免 IP 耗尽 ^[inferred]
- 配合 [[NetworkPolicy|NetworkPolicy]] 实现 Pod 间访问控制 ^[inferred]

## 参考链接

- [[cilium]]
- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[entities/cni-plugins.md|cni-plugins]]
- [[entities/networkpolicy.md|networkpolicy]]

## Related

- [[connect-rpc]] — Connect RPC
- [[antrea]] — Antrea
- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[cni]] — CNI (Container Network Interface)

- [[02-terway-architecture-deep-dive]]
- [[04-terway-crd-operations]]
- [[05-terway-operations-manual]]
- [[03-terway-usage-guide]]
- [[07-terway-performance-tuning]]
- [[06-terway-testing-validation]]
- [[08-terway-troubleshooting-fta]]
- 40-terway-product-overview

<!-- risk-assessed -->
