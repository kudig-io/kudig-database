---
title: 网络策略
description: NetworkPolicy 是 Kubernetes 中用于控制 Pod 之间以及 Pod 与外部网络之间流量访问的网络安全资源。它基于标签选择器定义允许/拒绝...
summary: NetworkPolicy 是 Kubernetes 中用于控制 Pod 之间以及 Pod 与外部网络之间流量访问的网络安全资源。它基于标签选择器定义允许/拒绝...
category: dictionary
tags:
- k8s
- glossary
- network-policy
- security
- networking
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 网络策略 是什么
- NetworkPolicy 详解
trigger_keywords:
- 网络策略
- NetworkPolicy
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 网络策略

> **英文名**: NetworkPolicy

## 概述

NetworkPolicy 是 Kubernetes 中用于控制 Pod 之间以及 Pod 与外部网络之间流量访问的网络安全资源。它基于标签选择器定义允许/拒绝的入站和出站规则。

## 核心概念/原理

### 核心概念

- **默认策略**：Kubernetes 默认允许所有 Pod 之间的通信（无隔离）。
- **策略生效**：为 Pod 配置 NetworkPolicy 后，未明确允许的流量将被拒绝。
- **三要素**：
  - `podSelector`：选择策略适用的 Pod。
  - `ingress`：定义允许的入站规则。
  - `egress`：定义允许的出站规则。

### 示例：限制只允许特定 Pod 访问

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
spec:
  podSelector:
    matchLabels:
      app: database
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: backend
    ports:
    - port: 5432
```

## 关键机制或特性

- NetworkPolicy 需要 CNI 插件支持（Calico、Cilium 等），部分 CNI 不支持。
- 策略基于 IP CIDR 和标签选择器，不直接支持 FQDN（域名）策略。
- 空 `ingress: []` 表示拒绝所有入站，空 `egress: []` 表示拒绝所有出站。

## 使用场景与最佳实践

- 生产环境应为所有应用配置 NetworkPolicy 实现最小权限网络访问。
- 从默认拒绝策略开始，逐步添加允许规则。
- 使用 Cilium 的 FQDN Policy 实现基于域名的出站控制。
- 定期审计 NetworkPolicy 覆盖情况，确保无遗漏。

## 架构深度解析

### NetworkPolicy 数据流

```
┌──────────────────────────────────────────────────────────────┐
│  Pod（带标签 app=backend）                                     │
│       │                                                       │
│       ▼                                                       │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ CNI 数据面（Calico felix / Cilium BPF / Antrea）         │  │
│  │  ├─ 入站：Pod 侧 veth/eth0 ingress 方向                   │  │
│  │  │   iptables INPUT 链（Calico）或 BPF 程序（Cilium）    │  │
│  │  ├─ 出站：Pod 侧 egress 方向                              │  │
│  │  └─ 策略存储：Calico 用 iptables 链 + IPSet，             │  │
│  │     Cilium 用 BPF map（Endpoint → PolicyMap）            │  │
│  └─────────────────────────────────────────────────────────┘  │
│       │                                                       │
│       ▼                                                       │
│  kube-apiserver（NetworkPolicy 对象 watch）                   │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ 默认行为：无策略 = 全部放行                               │  │
│  │ 有策略时：规则之间是 OR 关系（任一匹配即放行）            │  │
│  │ ingress: [] / egress: [] 空数组 = 全部拒绝               │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（projectcalico/felix 或 cilium/cilium）

| 模块 | 路径 | 职责 |
|------|------|------|
| 策略同步（Calico） | felix `rules/policy.go` | 将 NetworkPolicy 编译为 iptables 链规则 + IPSet |
| 策略计算（Cilium） | `pkg/policy/` | 基于 Endpoint 身份（Security Identity）编译 BPF 策略 |
| API 校验 | k8s `pkg/registry/networking/networkpolicy/` | 校验 spec 合法性（CIDR、端口、协议） |
| 标签选择器 | k8s `pkg/apis/networking/validation` | 将 podSelector/namespaceSelector 编译为 LabelSelector |

### 流程步骤

1. 用户创建 NetworkPolicy 对象，apiserver 校验并持久化。
2. CNI 控制器（Calico felix / Cilium operator）watch 到变更。
3. 选择器匹配目标 Pod（podSelector + namespaceSelector），计算受影响的 Endpoint 集合。
4. 数据面将策略编译为 iptables 规则（Calico）或 BPF 程序（Cilium），按 Endpoint 身份匹配。
5. 包到达时逐条评估：ingress 规则任一匹配 → 放行；全部不匹配 → 拒绝。

## 生产案例

### 案例 1：误配默认拒绝策略导致全集群断网

| 时间 | 事件 |
|------|------|
| 09:30 | 安全团队在 kube-system 命名空间应用默认拒绝策略 |
| 09:31 | CoreDNS、kube-proxy 等系统组件与外部通信全部中断 |
| 09:35 | 集群 DNS 解析失败，业务大面积报错 |
| 09:40 | `kubectl get networkpolicy -n kube-system` 定位到新策略 |
| 09:45 | 删除误配策略，集群逐步恢复 |
| 10:00 | 复盘：策略未覆盖系统组件所需端口（如 53/443 出站） |

**根因**：默认拒绝策略未为系统组件放行必要端口，且未先在测试命名空间验证。
**修复命令**：
```bash
# 查看所有 NetworkPolicy 🟢 只读
kubectl get networkpolicy -A
# 查看策略详情（确认选择器范围）🟢 只读
kubectl get networkpolicy <name> -n <ns> -o yaml
# 紧急删除误配策略 🟡 中风险
kubectl delete networkpolicy <name> -n <ns>
```

### 案例 2：服务间访问偶发失败（CIDR 与 IP 池不符）

**现象**：新扩容节点上的 Pod 无法访问数据库服务（其他节点正常）。
**诊断**：`kubectl get networkpolicy -o yaml` 显示 egress 规则使用硬编码 CIDR（旧 Pod CIDR）；新节点 Pod 网段未在策略中。
**修复**：将 CIDR 规则改为标签选择器（`podSelector`），或用 Cilium FQDN 策略；从设计上避免硬编码 IP 段。

## 对比评测

| 维度 | Calico (iptables) | Cilium (eBPF) | Antrea (OVS) |
|------|-------------------|---------------|--------------|
| 数据面 | iptables + IPset | BPF map | Open vSwitch |
| 性能 | 规则多时下降 | 恒定 O(1) | 中 |
| FQDN 策略 | 需 Enterprise | 支持（社区版） | 不支持 |
| 日志审计 | 弱 | 强（Hubble flow） | 中 |
| 适用规模 | 中小集群 | 大规模/高安全 | 需要 L2 特性 |

**选型建议**：安全合规要求高且需要流量审计选 Cilium；已有 Calico 且规模不大保持现状；Antrea 适合需要 OVS 生态的场景。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| Pod 间不通 | `kubectl get networkpolicy -A`；`calicoctl get profile` | 默认拒绝策略误配 |
| 出站失败 | 检查 egress 规则 CIDR | Pod 网段变更未同步 |
| 部分 Pod 受策略影响 | `kubectl get pods --show-labels` | 标签选择器匹配过宽/过窄 |
| DNS 解析失败 | 检查 kube-system 策略 | 53 端口未放行 |
| 策略不生效 | 确认 CNI 支持；`calicoctl get ippool` | CNI 不支持 NetworkPolicy |

## 生产部署清单

- [ ] 确认 CNI 支持 NetworkPolicy（Calico/Cilium/Antrea）
- [ ] 采用"默认拒绝 + 白名单"模式，先测试命名空间验证
- [ ] 系统组件（kube-system）策略已放行必要端口（53/443/6443 等）
- [ ] 避免硬编码 CIDR，优先标签选择器
- [ ] 建立策略审计机制（定期检查覆盖率和变更记录）

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 策略误配导致大面积断网 | 紧急删除/回滚，先恢复服务 |
| P1 | 需要 FQDN 出站控制 | 评估 Cilium 或 Calico Enterprise |
| P1 | 安全审计要求流量日志 | 启用 Cilium Hubble / 策略日志 |
| P2 | 策略稳定且覆盖达标 | 保持现状，纳入定期审计 |

## 面试要点

> 以下 Q&A 覆盖 NetworkPolicy 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：NetworkPolicy 的默认行为是什么？空数组（`ingress: []`）和没有该字段有什么区别？**
   A：没有 NetworkPolicy 时默认全部放行；一旦某 Pod 被任意 NetworkPolicy 选中，策略引擎开始评估：`ingress` 字段缺失表示该方向无限制（放行），`ingress: []` 空数组表示拒绝所有入站（因为没有任何规则匹配）。这个微妙区别是生产事故高发点，排查时先看字段是否存在。

2. **Q：NetworkPolicy 如何与 CNI 数据面配合实现流量控制？**
   A：NetworkPolicy 是 API 层声明，实际执行依赖 CNI：Calico 将策略编译为 iptables 链（每 Pod 一条链）+ IPset（按标签分组 IP），包经过 Pod 侧 veth 时按链匹配；Cilium 为每个 Endpoint 计算安全身份（Security Identity），将策略编译进 BPF map，包处理时 O(1) 查表决定放行/拒绝。因此 CNI 不支持 NetworkPolicy 时策略会被静默忽略。

3. **Q：如何设计一套安全的生产 NetworkPolicy 体系？**
   A：① 分层：集群级全局默认拒绝 + 命名空间级策略 + 应用级最小权限；② 先测试后生产：先在测试命名空间完整验证；③ 系统组件豁免：kube-system 放行 DNS（53）、kubelet（10250）、apiserver（6443）；④ 用标签而非 CIDR：避免 Pod 网段变更导致策略失效；⑤ 可观测：启用策略日志/Hubble 流量审计；⑥ 自动化：用 Kyverno/OPA 校验策略质量，禁止裸 Pod。

## 参考链接

- [NetworkPolicy - Official Documentation](https://kubernetes.io/docs/concepts/services-networking/network-policies/)

## Related

[[17-系统基础/06-知识字典/networking/network-policies.md|Network Policies]]


<!-- risk-assessed -->
