---
title: Terway 速览
description: Terway（阿里云 ACK CNI 插件）实体层速览卡——模式选型/核心依赖/高频命令/高频故障一页速查，深挖正文见主场
summary: Terway 通过 ENI 将 Pod 直接接入 VPC。本卡保留实体层一页速查（五种网络模式选型、核心依赖、高频命令、高频故障），原 8 张实体卡的深挖正文已并入主场 wiki/05-网络/06-Terway/。
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- eni
- ebpf
- networkpolicy
tier: peripheral
created: '2026-05-23'
last_updated: 2026-11
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 3min
intent_queries:
- Terway 速览
- Terway 网络模式怎么选
- Terway 高频故障有哪些
trigger_keywords:
- Terway
- 速览
prerequisites:
- kubectl-basics
---

# Terway 速览

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+
> **主场**: [[wiki/05-网络/06-Terway/index.md|Topic Terway 目录索引]] — 本卡为 S-1 重排后的实体层速览，原 8 张实体卡（产品概览/架构/使用/CRD/运维/测试/调优/FTA）的深挖正文已按主题并入主场。

## 主题导航（原 8 卡 → 主场）

| 主题 | 主场文件 |
|---|---|
| 产品概览（定位/模式/依赖） | [[wiki/05-网络/06-Terway/01-product.md\|01-product]] |
| 架构深潜（组件/拓扑/集成） | [[wiki/05-网络/06-Terway/02-architecture.md\|02-architecture]] |
| 使用指南（安装/配置） | [[wiki/05-网络/06-Terway/03-usage.md\|03-usage]] |
| CRD 操作（PodENI/MultiIP） | [[wiki/05-网络/06-Terway/03b-crd-operations.md\|03b-crd-operations]] |
| 运维手册（健康检查/诊断） | [[wiki/05-网络/06-Terway/04-operations.md\|04-operations]] |
| 测试验证（连通性用例） | [[wiki/05-网络/06-Terway/05-testing.md\|05-testing]] |
| 性能调优（模式对比/基准） | [[wiki/05-网络/06-Terway/06-performance.md\|06-performance]] |
| 故障排查（FTA 故障树） | [[wiki/05-网络/06-Terway/07-troubleshooting-fta.md\|07-troubleshooting-fta]] |

## 网络模式选型

| 模式 | Pod IP 来源 | 性能 (相对物理机) | 容量密度 | 适用场景 |
|:---|:---|:---:|:---:|:---|
| **VPC** | VPC 路由表条目 | ~70% | 低（路由 48 条限制） | 小规模、Flannel 迁移过渡 |
| **ENI** | 独占 ENI 主 IP | ~95% | 低（ENI 配额限制） | 核心数据库、网关 |
| **ENIIP** | ENI 辅助 IP | ~90% | 高（**推荐默认**） | 大规模通用、在线业务 |
| **ENIIP-Trunking** | Trunk ENI 辅助 IP | ~88% | 最高（单节点 200+ Pod） | 超大规模、Serverless |
| **IPVlan** | ENI 辅助 IP + IPVlan L2 | ~95% | 高 | 极致性能、低延迟 |

## 核心依赖

VPC（Pod 网络承载平面，vSwitch 分配 IP）→ ENI（网络接口载体）→ ECS OpenAPI（ENI/IP 分配释放）→ RAM 实例角色（临时凭证）。

## 高频命令

```bash
# 🟢 组件与 IP 分配状态
kubectl get pods -n kube-system -l app=terway-eniip
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli mapping
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli show factory

# 🟢 ENI 资源与 NetworkPolicy
kubectl get podeni -A -o wide
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli show policy

# 🟡 重启 Terway DaemonSet（网络短暂中断）
kubectl rollout restart daemonset/terway-eniip -n kube-system
```

## 高频故障

| 症状 | 可能原因 | 首查 |
|------|----------|------|
| Pod 卡 ContainerCreating | ENI/IP 分配失败 | `kubectl describe pod` Events → ENI 配额/vSwitch IP |
| Pod 无网络连通性 | 安全组规则限制 | 安全组入/出规则放行 Pod CIDR |
| 跨节点 Pod 不通 | VPC 路由缺失 | VPC 路由表补 Pod CIDR 路由 |
| IP 地址耗尽 | vSwitch IP 不足 | 扩容 vSwitch/多 vSwitch 分散 |
| NetworkPolicy 不生效 | eBPF 未启用 | 检查 Terway 配置启用 eBPF |

完整 FTA 故障树见主场 [[wiki/05-网络/06-Terway/07-troubleshooting-fta.md|07-troubleshooting-fta]]。

## Related

- [[wiki/05-网络/06-Terway/terway-index.md|terway-index]] — Terway 全项目资源索引
- [[cilium]] — Cilium
- [[cni]] — CNI (Container Network Interface)
- [[wiki/23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[wiki/23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]] — CNI 插件对比

<!-- risk-assessed -->
