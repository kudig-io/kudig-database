---
title: "网络自愈自动化"
description: "网络自愈自动化：网络健康检测、自动修复、CNI 故障恢复、DNS 故障切换与自愈闭环"
summary: "面向 SRE 的 Kubernetes 网络自愈完整指南，覆盖网络健康检测、CNI 故障自动恢复、DNS 故障切换、自愈控制器与闭环自动化设计。"
category: 网络
tags:
- self-healing
- networking
- cni
- automation
- dns
- resilience
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 网络工程师
estimated_read_time: 20min
intent_queries:
- "Kubernetes 网络故障如何自动恢复"
- "CNI 故障如何自愈"
- "如何构建网络自愈闭环"
trigger_keywords:
- self-healing
- network automation
- cni recovery
- dns failover
- auto-remediation
prerequisites:
- kubectl-basics
- networking-basics
- cni-fundamentals
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 网络自愈自动化

> **适用版本**: Kubernetes v1.28+
> **最后更新**: 2026-07

---

## 概述

网络是 Kubernetes 中最复杂、最容易出现故障的层面。CNI 插件崩溃、节点路由表损坏、CoreDNS 不可用、ipvs 规则残留、隧道端点失联——这些故障在生产环境中并不罕见，而每一次网络故障都可能导致大面积的服务不可用。如果这些故障都依赖人工介入处理，平均恢复时间（MTTR）可能长达数十分钟甚至数小时，对于在线业务来说这是不可接受的。

网络自愈自动化的目标是：通过持续的健康检测加上自动修复闭环，将常见网络故障的恢复时间从分钟级压缩到秒级，并且全程无需人工干预。这不是一个乌托邦式的愿景——在我们的生产实践中，通过合理的自愈设计，超过 80% 的网络故障可以在 30 秒内自动恢复，只有少数复杂故障需要人工介入。

但自愈也伴随着风险。自动修复最大的敌人不是技术实现，而是误判——如果检测逻辑有缺陷，自愈系统可能在网络正常时执行不必要的修复操作，反而制造故障。因此，安全护栏的设计与修复逻辑本身同等重要。本文系统覆盖网络健康检测体系、CNI 故障自动恢复、DNS 故障切换、以及自愈控制器的设计与安全护栏。CNI 故障的手动排查见 [[05-网络/01-K8s网络核心/28-cni-troubleshooting-optimization.md|CNI 故障排查与优化]]，DNS 故障见 [[05-网络/01-K8s网络核心/29-coredns-troubleshooting-optimization.md|CoreDNS 故障排查]]。

---

## 核心概念

### 1. 自愈闭环模型

一个完整的网络自愈系统遵循"检测-诊断-决策-修复-验证"的五阶段闭环模型，每个阶段都有其独特的设计考量。

```
检测 (Detect) → 诊断 (Diagnose) → 决策 (Decide) → 修复 (Remediate) → 验证 (Verify)
    ↑                                                                    │
    └──────────────────────  反馈循环  ──────────────────────────────────┘
```

| 环节 | 关键问题 | 实现手段 |
|------|---------|---------|
| 检测 | 如何快速发现故障 | 主动探测 + 被动指标 + 事件 |
| 诊断 | 如何定位根因 | 故障树 + 模式匹配 |
| 决策 | 是否自动修复 | 风险分级 + 护栏 |
| 修复 | 如何安全修复 | 分级修复动作 |
| 验证 | 修复是否成功 | 复检 + 回滚 |

检测环节是整个闭环的起点，也是最容易被低估的环节。一个好的检测系统应该同时包含三种手段：主动探测（定期发送探测包验证连通性）、被动指标（监控错误率、延迟等指标的变化）、事件监听（捕获 Pod 创建失败、CNI 错误等 Kubernetes 事件）。单一手段都有盲区，三者结合才能实现全面的故障感知。

### 2. 网络故障分类与自愈策略

不同类型的网络故障，其影响范围和自愈难度差异巨大，需要采用不同的策略。

| 故障类型 | 影响 | 自愈难度 | 策略 |
|---------|------|---------|------|
| 单 Pod 网络异常 | 局部 | 低 | 重建 Pod |
| CNI Pod 崩溃 | 节点 | 中 | 重启 CNI Pod |
| 节点路由损坏 | 节点 | 中 | 重置网络/重启 CNI |
| CoreDNS 不可用 | 全集群 | 高 | 切换 + 扩容 |
| ipvs/iptables 残留 | 节点 | 中 | 清理规则 |
| 跨节点不通 | 多节点 | 高 | 隔离节点 + 告警 |

自愈策略的核心原则是"分级响应"：影响范围越小、修复风险越低的操作越可以大胆自动化；影响范围越大、修复风险越高的操作越需要谨慎，甚至应该转为人工处理。单 Pod 网络异常可以毫不犹豫地自动重建，但跨节点的网络不通可能涉及底层物理网络问题，盲目自动修复可能适得其反。

### 3. 自愈的安全护栏

安全护栏是区分"可靠的自愈系统"和"危险的自动化脚本"的关键。没有护栏的自愈系统，其造成的故障可能比它修复的故障还多。

爆炸半径限制是最基本的护栏：单次自愈操作最多影响 N 个节点或 Pod，防止一个错误的检测逻辑导致全集群范围的破坏性操作。冷却期机制确保同一个对象在被修复后的一段时间内不会被再次修复，防止检测抖动导致的修复循环。熔断机制是最后的安全网：如果在短时间内故障数量超过阈值，说明可能是系统级问题而非个别故障，此时应停止自动修复并告警人工介入。所有自愈动作都必须记录详细的审计日志，包括触发原因、执行的操作和最终结果，这是事后复盘和持续改进的基础。

---

## 生产部署/实现

### 1. 网络健康检测（主动探测） 🟢

主动探测是网络健康检测的第一道防线。通过在每个节点部署探测 Agent，定期验证关键网络路径的连通性。

```yaml
# 🟢 低风险：部署网络探测 DaemonSet
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: net-health-probe
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: net-health-probe
  template:
    metadata:
      labels:
        app: net-health-probe
    spec:
      hostNetwork: true
      tolerations:
      - operator: Exists
      containers:
      - name: probe
        image: registry.example.com/net-probe:v1.0
        env:
        - name: NODE_NAME
          valueFrom:
            fieldRef:
              fieldPath: spec.nodeName
        - name: PROBE_TARGETS
          value: "10.96.0.1:443,10.96.0.10:53"   # apiserver, coredns
        - name: PROBE_INTERVAL
          value: "10s"
        securityContext:
          privileged: false
          capabilities:
            add: ["NET_RAW", "NET_ADMIN"]
```

这个 DaemonSet 使用 hostNetwork 直接在节点网络命名空间中运行探测，能够真实反映节点级别的网络连通性。PROBE_TARGETS 配置了关键的探测目标：apiserver 的 Service IP（验证 Service 网络是否正常）和 CoreDNS 的 Service IP（验证 DNS 服务是否可达）。探测间隔设为 10 秒，在及时发现故障和避免探测流量过大之间取得平衡。NET_RAW 和 NET_ADMIN 能力允许发送原始探测包，但不需要完整的 privileged 权限。

### 2. CNI 故障自动恢复控制器 🟡

自愈控制器是整个系统的核心，它接收检测结果、做出修复决策、执行修复操作。

```yaml
# 🟡 中风险：自动修复控制器，需谨慎配置 RBAC 与护栏
apiVersion: apps/v1
kind: Deployment
metadata:
  name: net-self-healing-controller
  namespace: kube-system
spec:
  replicas: 2
  selector:
    matchLabels:
      app: net-self-healing
  template:
    metadata:
      labels:
        app: net-self-healing
    spec:
      serviceAccountName: net-healer
      containers:
      - name: controller
        image: registry.example.com/net-healer:v1.0
        env:
        # 护栏配置
        - name: MAX_CONCURRENT_REMEDIATIONS
          value: "2"               # 最多同时修复 2 个节点
        - name: COOLDOWN_SECONDS
          value: "300"             # 同节点冷却 5 分钟
        - name: CIRCUIT_BREAKER_THRESHOLD
          value: "5"               # 10 分钟内超 5 个节点故障则熔断
        - name: DRY_RUN
          value: "false"           # 上线前先 true 观察
```

修复逻辑遵循分级响应原则：

```python
# 🟡 中风险：自愈决策逻辑
def remediate_node(node):
    if not pass_guardrails(node):       # 护栏检查
        alert_human("熔断或超爆炸半径")
        return
    diagnose = detect_fault_type(node)
    if diagnose == "CNI_POD_CRASH":
        restart_cni_pod(node)           # 级别1：重启 CNI
    elif diagnose == "ROUTE_CORRUPTED":
        reset_node_network(node)        # 级别2：重置网络
    elif diagnose == "NODE_NETWORK_DOWN":
        cordon_and_alert(node)          # 级别3：隔离 + 人工
    verify_and_record(node)             # 验证 + 审计
```

这个分级逻辑体现了"能轻不重"的原则：CNI Pod 崩溃是最常见的故障，重启 CNI Pod 就能解决，影响最小；路由表损坏需要更重的操作——重置节点网络，这会短暂影响该节点上所有 Pod 的连通性；而节点网络完全不可达可能是底层硬件或物理网络问题，自动修复无能为力，应该隔离节点并告警人工处理。DRY_RUN 模式是上线前的必备验证手段——在 DRY_RUN 模式下，控制器会执行完整的检测和诊断流程，但不会真正执行修复操作，只记录"如果不在 DRY_RUN 模式下会做什么"，让运维团队评估自愈逻辑的准确性。

### 3. DNS 故障切换（NodeLocal + CoreDNS 高可用） 🟡

DNS 是网络自愈中最重要的环节之一，因为 DNS 不可用会导致全集群范围的服务发现失败。

```yaml
# 🟡 中风险：NodeLocal DNSCache 提供本地 DNS 容错
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-local-dns
  namespace: kube-system
spec:
  template:
    spec:
      hostNetwork: true
      containers:
      - name: node-cache
        image: registry.k8s.io/dns/k8s-dns-node-cache:1.23.1
        args:
        - -localip
        - 169.254.20.10
        - -conf
        - /etc/Corefile
        - -upstreamsvc
        - kube-dns
        securityContext:
          capabilities:
            add: ["NET_ADMIN"]
        volumeMounts:
        - name: xtables-lock
          mountPath: /run/xtables.lock
```

NodeLocal DNSCache 是 DNS 高可用的基石。它在每个节点上运行一个本地 DNS 缓存，Pod 的 DNS 查询首先到达本节点的缓存，只有缓存未命中时才会转发到上游的 CoreDNS。这种架构带来了三重好处：大幅降低 DNS 查询延迟（本地缓存响应在微秒级）、消除 conntrack UDP 竞态问题（本地查询不经过 conntrack）、提供 CoreDNS 故障时的容错能力（缓存中的记录在上游不可用时仍可响应）。

Pod 配置指向本地 DNS（故障时自动回退到上游 CoreDNS）：

```yaml
# 🟡 中风险
dnsConfig:
  nameservers:
  - 169.254.20.10        # NodeLocal DNSCache
  - 10.96.0.10           # 上游 CoreDNS（回退）
  searches:
  - production.svc.cluster.local
  - svc.cluster.local
  - cluster.local
  options:
  - name: ndots
    value: "5"
```

---

## 运维操作

### 1. 网络健康巡检 🟢

定期的网络健康巡检是发现潜在问题的重要手段，应该在故障发生之前识别风险。

```bash
# 🟢 低风险：只读巡检脚本
#!/bin/bash
echo "=== CNI Pod 状态 ==="
kubectl -n kube-system get pods -l k8s-app=calico-node -o wide | grep -v Running

echo "=== CoreDNS 状态 ==="
kubectl -n kube-system get pods -l k8s-app=kube-dns

echo "=== 节点网络就绪 ==="
kubectl get nodes -o custom-columns=NAME:.metadata.name,READY:.status.conditions[-1].type

echo "=== 最近网络相关事件 ==="
kubectl get events -A --field-selector reason=FailedCreatePodSandBox | tail -10
```

这个巡检脚本检查了网络健康的几个关键维度：CNI Pod 是否全部运行正常、CoreDNS 是否健康、节点是否处于 Ready 状态、以及是否有 Pod 因为网络原因创建失败。建议将其集成到定时任务中，每天自动执行并报告异常。

### 2. 手动触发 CNI 恢复 🔴

当自动自愈无法解决问题时，可能需要手动介入恢复 CNI。

```bash
# 🔴 高风险：重启 CNI 会短暂中断节点上 Pod 网络
NODE=worker-03
# 1. 先 cordon 防止新 Pod 调度
kubectl cordon $NODE

# 2. 重启该节点 CNI Pod
kubectl -n kube-system delete pod -l k8s-app=calico-node --field-selector spec.nodeName=$NODE

# 3. 等待 CNI 就绪后 uncordon
kubectl -n kube-system wait pod -l k8s-app=calico-node --field-selector spec.nodeName=$NODE --for=condition=Ready --timeout=120s
kubectl uncordon $NODE
```

手动 CNI 恢复的关键是先 cordon 再操作。cordon 确保在 CNI 重启期间不会有新 Pod 调度到该节点，避免新 Pod 因为 CNI 不可用而创建失败。重启 CNI Pod 会短暂中断该节点上现有 Pod 的网络连接（通常几秒到十几秒），因此这个操作应该在业务低峰期执行，或者在有多副本冗余的服务上进行。

### 3. 自愈审计查询 🟢

```bash
# 🟢 低风险
kubectl -n kube-system logs deploy/net-self-healing-controller --tail=200 | grep -i "remediat"
kubectl get events -n kube-system --field-selector reason=NetworkSelfHealed
```

---

## 故障排查

### 症状 1：Pod 创建失败 FailedCreatePodSandBox

```bash
# 🟢 低风险
kubectl describe pod <pod> | grep -A10 Events
kubectl -n kube-system logs ds/calico-node | grep -i error
```

FailedCreatePodSandBox 是 CNI 故障最典型的表现。根因可能是 CNI 插件进程异常、IPAM 地址池耗尽、或者节点的网络命名空间损坏。处置方法是重启 CNI Pod、检查 IPAM 池的剩余地址数、必要时重置节点网络。

### 症状 2：节点间 Pod 不通

根因可能是节点路由表损坏（如 CNI 写入的路由被意外删除）、CNI 隧道（VXLAN/IPIP）中断、或者 MTU 不匹配导致大包被丢弃。处置方法是检查 ip route 输出是否包含到其他节点 Pod 网段的路由、重启 CNI 重建隧道、核对所有节点的 MTU 配置是否一致。

### 症状 3：自愈控制器误修复

根因是检测阈值过于敏感（如将短暂的网络抖动误判为故障）、或者护栏配置缺失。处置方法是调整检测阈值增加容忍度、启用 DRY_RUN 模式观察一段时间、增加冷却期和熔断阈值。

### 症状 4：DNS 间歇不可用

根因是 CoreDNS 副本不足在高峰期过载、或者 conntrack 竞态导致 UDP 查询丢失。处置方法是部署 NodeLocal DNSCache、扩容 CoreDNS 副本，参考 [[05-网络/01-K8s网络核心/52-dns-advanced-external-integration.md|DNS 高级与外部集成]]。

### 排查决策树

```
网络故障
├── Pod 创建失败?   → CNI/IPAM → 重启 CNI
├── 节点间不通?     → 路由/隧道/MTU
├── DNS 不可用?     → CoreDNS/NodeLocal
└── 自愈误判?       → 阈值/护栏/DRY_RUN
```

---

## 最佳实践

第一，检测要分层，主动探测（连通性）、被动指标（错误率）、事件监听（Pod 失败）三维结合，避免单一手段的盲区。第二，修复要分级，从低风险操作（重启 Pod）到高风险操作（重置节点网络）逐级升级，避免过度反应。第三，护栏是必备而非可选，爆炸半径限制、冷却期、熔断、DRY_RUN 上线观察，四者缺一不可。第四，DNS 容错通过 NodeLocal DNSCache 加 CoreDNS 多副本实现，确保单点故障不影响全集群解析。第五，所有自愈动作必须记录事件和日志，便于事后复盘和持续优化。第六，定期通过混沌工程注入网络故障，验证自愈机制的有效性，参考 [[12-可靠性/04-混沌工程/index|04-混沌工程]]。第七，高风险故障（跨节点、控制平面相关）采用自动隔离加告警人工的策略，不盲目自动修复。

```yaml
# 🟢 低风险：自愈动作告警通知
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: net-healing-alerts
spec:
  groups:
  - name: self-healing
    rules:
    - alert: NetworkSelfHealingCircuitBreaker
      expr: net_healer_circuit_breaker_open == 1
      labels:
        severity: critical
      annotations:
        summary: "网络自愈熔断已触发，需人工介入"
    - alert: CNIPodCrashLooping
      expr: rate(kube_pod_container_status_restarts_total{namespace="kube-system",pod=~".*calico.*|.*cilium.*"}[10m]) > 0
      for: 5m
      labels:
        severity: warning
```

---

## Related

- [[05-网络/01-K8s网络核心/28-cni-troubleshooting-optimization.md|CNI 故障排查与优化]]
- [[05-网络/01-K8s网络核心/29-coredns-troubleshooting-optimization.md|CoreDNS 故障排查]]
- [[05-网络/01-K8s网络核心/52-dns-advanced-external-integration.md|DNS 高级与外部集成]]
- [[05-网络/01-K8s网络核心/03-cni-architecture-fundamentals.md|CNI 架构基础]]
- [[05-网络/01-K8s网络核心/35-network-troubleshooting.md|网络故障排查]]
- [[12-可靠性/04-混沌工程/index|04-混沌工程]]
