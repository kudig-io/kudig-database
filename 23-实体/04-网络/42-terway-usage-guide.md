---
title: Terway 使用指南
description: '# Terway 使用指南'
summary: 'Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。^[inferred]'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- istio
- cilium
- networkpolicy
- crd
- ebpf
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 使用指南 是什么
- 如何 Terway 使用指南
trigger_keywords:
- Terway
- 使用指南
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Terway 使用指南

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

Terway 是阿里云 ACK（Aliyun Container Service for Kubernetes）的默认 CNI 插件。它通过 ENI（Elastic Network Interface）将 Pod 直接接入阿里云 VPC（Virtual Private Cloud）网络，使 Pod 拥有 VPC 内的真实 IP 地址，无需 NAT 转换。本页涵盖 Terway 的日常使用操作——从网络模式选择、IPAM 管理、NetworkPolicy 配置到与 Service Mesh 集成。

Terway 提供两种网络模式供不同场景选择：**ENI 多 IP 模式**（共享 ENI，默认，一个 ENI 的多个辅助 IP 分配给不同 Pod）和**ENI 独占模式**（每个 Pod 绑定一个完整 ENI，性能最高但 IP 消耗大）。用户通过 `pod-template` annotation 或 ` terway-config` ConfigMap 选择模式。

## 核心操作

- **网络模式选择**：共享 ENI（默认，高密度）vs 独占 ENI（高性能）
- **IPAM 管理**：辅助 IP 的分配、回收和配额管理
- **NetworkPolicy 配置**：Pod 间访问控制（iptables 或 eBPF 引擎）
- **EIP 绑定**：为 Pod 绑定弹性公网 IP（用于对外暴露服务）
- **安全组管理**：为 Pod 配置独立安全组（通过 SecurityGroup CRD）
- **Service Mesh 集成**：与 Istio/ASM 配合处理流量管理

## Architecture

Terway 的 `terway-daemon` 运行在每个节点上，通过阿里云 OpenAPI 管理该节点的 ENI 和辅助 IP。当 Pod 创建时，terway-daemon 从本地 IPAM 池中分配一个 VPC IP，创建 Veth Pair 连接 Pod 网络命名空间和节点网络栈，配置路由规则使 Pod 流量通过 ENI 到达 VPC。NetworkPolicy 规则由 `terway-daemon` 编译为 iptables 规则或 eBPF 程序应用到节点。

## K8s 集成

Terway 作为标准 CNI 插件运行。kubelet 在创建 Pod 时调用 Terway CNI binary 进行网络配置。通过 CRD（`NetworkPolicy`、`SecurityGroup`）扩展网络管理能力。支持通过 annotation 自定义单个 Pod 的网络配置（如 `k8s.aliyun.com/eni-type: eni` 指定独占 ENI）。

## 生产部署要点

- **ENI 模式选择**：默认使用 ENI 多 IP 模式，性能敏感场景使用独占 ENI
- **IP 容量规划**：根据节点规格计算可用 ENI 和辅助 IP 数量，避免 IP 耗尽
- **安全组策略**：为不同业务 Pod 配置独立安全组，实现网络隔离
- **NetworkPolicy**：使用 eBPF 模式提升大规模 NetworkPolicy 性能

## 生产场景

1. **标准微服务部署**：使用共享 ENI 模式，Pod 获得 VPC IP 与其他服务通信
2. **高性能计算**：AI/HPC 工作负载使用独占 ENI 获得最大网络吞吐
3. **网络隔离**：通过 SecurityGroup CRD 为不同租户的 Pod 配置独立安全组
4. **对外暴露**：通过 EIP 绑定为特定 Pod 提供公网访问能力

## 操作命令

```bash
# 🟢 查看 Terway 配置
kubectl get cm eni-config -n kube-system -o yaml
kubectl get cm terway-config -n kube-system -o yaml

# 🟢 查看节点 ENI 分配情况
kubectl get node <node-name> -o jsonpath='{.metadata.annotations}' | jq | grep eni

# 🟢 查看 Pod 网络信息
kubectl get pod <pod-name> -o jsonpath='{.metadata.annotations}' | jq | grep networking

# 🟡 为 Pod 指定独占 ENI 模式
kubectl annotate pod <pod-name> k8s.aliyun.com/eni-type=eni

# 🟢 创建安全组 CRD
kubectl apply -f - <<EOF
apiVersion: network.alibabacloud.com/v1beta1
kind: SecurityGroup
metadata:
  name: my-sg
spec:
  securityGroupID: sg-xxxxx
EOF

# 🟢 检查 Terway Pod 状态
kubectl get pod -n kube-system -l app=terway -o wide
kubectl logs -n kube-system terway-daemon-xxxxx --tail=20
```

## 对比

| 特性 | Terway | Cilium | Calico | Flannel |
|------|--------|--------|--------|---------|
| VPC 原生 IP | ✅ 阿里云 VPC | ❌ | ❌ | ❌ |
| ENI 独占 | ✅ | ❌ | ❌ | ❌ |
| 安全组集成 | ✅ | ❌ | ❌ | ❌ |
| 适用云 | 阿里云 | 通用 | 通用 | 通用 |

## 参考链接

- [[istio]]
- [[cilium]]
- [[22-概念/03-网络/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]

## Related

- [[bfe]] — BFE
- [[26-技能/03-节点/node/skill-notready/skill-k8s-node-notready-USAGE-GUIDE.md|skill-k8s-node-notready-USAGE-GUIDE]] — Usage Guide
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[41-terway-architecture-deep-dive]]
- [[43-terway-crd-operations]]
- [[44-terway-operations-manual]]
- [[40-terway-product-overview]]
- [[46-terway-performance-tuning]]
- [[45-terway-testing-validation]]
- [[47-terway-troubleshooting-fta]]
- 42-terway-usage-guide

<!-- risk-assessed -->
