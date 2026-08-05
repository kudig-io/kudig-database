---
title: Terway CRD 资源操作
description: '## 概述'
summary: 'Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。^[inferred]'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
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
- Terway CRD 资源操作 是什么
- 如何 Terway CRD 资源操作
trigger_keywords:
- Terway
- CRD
- 资源操作
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Terway CRD 资源操作

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

Terway 除了标准的 CNI 网络配置功能外，还通过自定义 CRD（Custom Resource Definition）扩展了阿里云特有的网络管理能力。本页深入介绍 Terway 的核心 CRD 资源——从安全组管理（SecurityGroup）、网络策略扩展（NetworkPolicy）到 Pod 级别的 EIP 绑定和 QoS 配置。

Terway CRD 资源让用户可以通过声明式 Kubernetes API 管理阿里云网络资源——创建 `SecurityGroup` CR 自动同步到阿里云安全组，创建 `NetworkPolicy` CR 编译为节点上的 eBPF/iptables 规则。这些 CRD 扩展了标准 Kubernetes 网络模型，提供了云原生网络安全管理能力。

## 核心 CRD 资源

- **SecurityGroup**：将阿里云安全组绑定到特定 Pod/命名空间，实现云原生网络安全隔离
- **NetworkPolicy**：标准 K8s NetworkPolicy 的扩展，支持更丰富的匹配条件和动作
- **PodNetworkPolicy**：Pod 级别的网络策略，覆盖命名空间级策略
- **EIPBinding**：为 Pod 绑定弹性公网 IP（EIP），实现 Pod 级别的公网访问
- **ReservedIP**：预留 IP 地址，确保特定 Pod 总是获得固定 IP

## Architecture

Terway CRD 通过 **terway-controller**（管理集群级 CRD 资源）和 **terway-daemon**（在节点上执行实际网络配置）协同工作。用户创建 CRD 资源后，terway-controller Watch 到变更，通过阿里云 API 或节点配置实现期望状态。例如创建 `SecurityGroup` CR 时，controller 调用阿里云 API 创建/查询安全组，然后将安全组 ID 下发到相关节点的 terway-daemon。

## K8s 集成

Terway CRD 遵循标准 Kubernetes CRD 规范。用户通过 `kubectl apply` 创建/更新 CR 资源。CRD 的 status 字段反映实际同步状态。可以通过 annotation 将 CR 关联到特定 Pod 或命名空间（如 `k8s.aliyun.com/security-group: sg-xxx`）。RBAC 权限控制用户对 CRD 资源的访问。

## 生产部署要点

- **安全组规划**：为不同业务/租户规划独立安全组，通过 CRD 统一管理
- **NetworkPolicy 优先级**：注意策略优先级，更具体的策略优先于通用策略
- **EIP 管理**：EIP 是有限资源，绑定后及时回收
- **ReservedIP 审计**：预留 IP 影响容量规划，定期审计使用情况

## 生产场景

1. **多租户网络隔离**：不同租户的 Pod 通过不同 SecurityGroup CR 实现网络隔离
2. **Pod 公网访问**：特定 Pod 通过 EIPBinding CR 绑定 EIP，实现公网入站
3. **固定 IP 应用**：有状态应用通过 ReservedIP CR 保持重启后 IP 不变
4. **细粒度策略**：通过扩展 NetworkPolicy CR 实现标准 K8s NP 不支持的高级匹配

## 操作命令

```bash
# 🟢 列出所有 Terway CRD
kubectl get crd | grep -i terway
kubectl get crd | grep -i network.alibabacloud

# 🟢 创建安全组 CRD
kubectl apply -f - <<EOF
apiVersion: network.alibabacloud.com/v1beta1
kind: SecurityGroup
metadata:
  name: prod-sg
spec:
  securityGroupID: sg-xxxxxxxxx
  selectors:
    namespaceSelector:
      matchLabels:
        env: production
EOF

# 🟢 为 Pod 绑定安全组（通过 annotation）
kubectl annotate pod <pod-name> k8s.aliyun.com/security-group="sg-xxx,sg-yyy"

# 🟢 创建 EIP 绑定 CRD
kubectl apply -f - <<EOF
apiVersion: network.alibabacloud.com/v1beta1
kind: EIPBinding
metadata:
  name: web-eip
spec:
  eipId: eip-xxxxxxxxx
  podSelector:
    matchLabels:
      app: web-frontend
EOF

# 🟢 查看 CRD 状态
kubectl get securitygroup -A
kubectl get eipbinding -A
kubectl describe securitygroup prod-sg
```

## 对比

| CRD 能力 | Terway | Cilium | Calico |
|----------|--------|--------|--------|
| 安全组集成 | ✅ 阿里云 SG | ❌ | ❌ |
| EIP 绑定 | ✅ | ❌ | ❌ |
| ReservedIP | ✅ | ⚠️ | ✅ |
| 策略扩展 | ✅ | ✅ CiliumNetworkPolicy | ✅ GlobalNetworkPolicy |

## 参考链接

- [[cilium]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]

## Related

- [[03-terway-product-overview]] — Terway 产品概览
- [[07-terway-operations-manual]] — Terway 运维手册
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[cni]] — CNI (Container Network Interface)

- [[04-terway-architecture-deep-dive]]
- [[05-terway-usage-guide]]
- [[09-terway-performance-tuning]]
- [[08-terway-testing-validation]]
- [[10-terway-troubleshooting-fta]]
- 43-terway-crd-operations

<!-- risk-assessed -->
