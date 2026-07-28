---
title: NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting (skills)
description: '| S1 | 新部署 Pod 无法访问已有服务 | `kubectl exec` 测试连通性 | 0.85 | 应用启动失败 → SKILL-POD-001
  |'
summary: '| S1 | 新部署 Pod 无法访问已有服务 | `kubectl exec` 测试连通性 | 0.85 | 应用启动失败 → SKILL-POD-001
  |'
category: skills
tags:
- k8s
- troubleshooting
- skill
- networkpolicy
- 网络策略
- policy blocking
- 网络不通
- 连接被拒绝
- prometheus
- cilium
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting 是什么
- 如何 NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting
trigger_keywords:
- NetworkPolicy
- 连通性故障诊断
- NetworkPolicy
- Connectivity
- Troubleshooting
prerequisites:
- kubectl-basics
- prometheus-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[networkpolicy|NetworkPolicy]] 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting

### 症状识别



### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | 新部署 Pod 无法访问已有服务 | `kubectl exec` 测试连通性 | 0.85 | 应用启动失败 → SKILL-POD-001 |
| S2 | NetworkPolicy 变更后通信中断 | 检查事件时间线 | 0.90 | 同时有其他变更 |
| S3 | 跨 namespace Pod 间通信失败 | 跨 namespace `kubectl exec` 测试 | 0.85 | DNS 问题 → SKILL-NET-001 |
| S4 | Cilium/Calico policy denied 指标升高 | Prometheus/Hubble 观测 | 0.95 | 攻击流量 → SKILL-SECURITY-001 |
| S5 | 入站正常但出站失败（或相反） | 定向连通性测试 | 0.80 | [[service\|Service]] 问题 → SKILL-NET-002 |
| S6 | 所有 NetworkPolicy 似乎不生效 | 策略存在但流量未被过滤 | 0.85 | CNI 不支持 → RC-001 |
| S7 | 特定标签 Pod 通信被阻断 | 按标签分组测试连通性 | 0.85 | Pod 本身问题 → SKILL-POD-001 |
| S8 | 默认 namespace 通信正常，新 namespace 失败 | 多 namespace 对比测试 | 0.80 | namespace 配置问题 |

### 诊断工作流



### Phase 1: 快速检查（只读，零风险）

> **目标**: 通过 kubectl 远程收集 NetworkPolicy 和 Pod 信息，无需进入 Pod。
> **预计耗时**: 2-5 分钟

**Step D1.1**: 列出目标 namespace 的所有 NetworkPolicy
- **命令**:
  ```bash
  kubectl get networkpolicy -n <namespace> -o wide
  kubectl get networkpolicy -n <namespace> -o json | \
    jq '.items[] | {name: .metadata.name, types: .spec.policyTypes, ingress: (.spec.ingress | length), egress: (.spec.egress | length)}'
  ```
- **超时**: 10s
- **预期输出模式**: NetworkPolicy 列表及基本配置
- **判断规则**:
  - 存在 `policyTypes: ["Ingress"]` 但无 `ingress` 规则 → 默认拒绝所有入站（RC-002）
  - 存在 `policyTypes: ["Egress"]` 但无 `egress` 规则 → 默认拒绝所有出站（RC-002）
  - 多个 NetworkPolicy 同时存在 → 可能规则冲突（RC-006）
  - 无任何 NetworkPolicy → 若环境要求策略管控，则可能是策略未部署
- **版本差异**: 无

**Step D1.2**: 检查受影响 Pod 的标签
- **命令**:
  ```bash
  kubectl get pod <src-pod> -n <src-namespace> -o jsonpath='{.metadata.labels}' | jq .
  kubectl get pod <target-pod> -n <target-namespace> -o jsonpath='{.metadata.labels}' | jq .
  ```
- **超时**: 10s
- **预期输出模式**: Pod 标签 JSON
- **判断规则**:
  - 标签与 NetworkPolicy 的 `podSelector` 不匹配 → RC-003（标签选择器错误）
  - 标签值拼写错误（如 `app=frontend` vs `app=frontned`）→ RC-003
- **版本差异**: 无

**Step D1.3**: 检查 NetworkPolicy 详细规则
- **命令**:
  ```bash
  kubectl describe networkpolicy <policy-name> -n <namespace>
  kubectl get networkpolicy <policy-name> -n <namespace> -o yaml
  ```
- **超时**: 10s
- **预期输出模式**: NetworkPolicy 完整 YAML
- **判断规则**:
  - `namespaceSelector` 未匹配源/目标 namespace → RC-004（跨 namespace 问题）
  - `podSelector` 在 `from`/`to` 中未正确设置 → RC-003
  - `ports` 字段缺失或端口不匹配 → RC-005（规则不完整）
  - `protocol` 字段缺失（默认 TCP，若应用使用 UDP 则失败）→ RC-005
  - IPBlock CIDR 格式错误 → RC-005
- **版本差异**: 无

**Step D1.4**: 检查 namespace 标签（用于 namespaceSelector）
- **命令**:
  ```bash
  kubectl get namespace <namespace> -o jsonpath='{.metadata.labels}' | jq .
  ```
- **超时**: 5s
- **预期输出模式**: namespace 标签 JSON
- **判断规则**:
  - namespace 无 NetworkPolicy 所需的标签 → RC-004（跨 namespace 匹配失败）
  - namespace 标签值拼写错误 → RC-004
- **版本差异**: 无

**Step D1.5**: 检查 CNI 插件类型和支持情况
- **命令**:
  ```bash
  # 检查节点上的 
...(截断)

### Phase 2: 深度检查（只读，零风险）

> **目标**: 深入分析策略规则匹配情况和 CNI 内部状态。
> **预计耗时**: 5-15 分钟

**Step D2.1**: 分析 NetworkPolicy 的精确匹配逻辑
- **命令**:
  ```bash
  # 获取所有可能相关的 NetworkPolicy
  kubectl get networkpolicy -n <namespace> -o json | jq -r '
    .items[] |
    "Policy: \(.metadata.name) | Types: \(.spec.policyTypes) | " +
    "PodSelector: \(.spec.podSelector) | " +
    "IngressRules: \(.spec.ingress // [] | length) | " +
    "EgressRules: \(.spec.egress // [] | length)"
  '
  ```
- **超时**: 10s
- **预期输出模式**: 格式化策略信息
- **判断规则**:
  - Pod 被多个 NetworkPolicy 覆盖 → RC-006（规则冲突）
  - 一个策略允许，另一个策略拒绝同一流量 → RC-006（NetworkPolicy 是累积允许，不冲突时需确认）
  - 注意：NetworkPolicy 的默认行为是 "无策略 = 允许所有"，"有策略 = 只允许规则中定义"
- **版本差异**: 无

**Step D2.2**: 检查 Calico NetworkPolicy 状态（如使用 Calico）
- **命令**:
  ```bash
  # Calico 全局网络策略
  kubectl get globalnetworkpolicy 2>/dev/null
  # Calico 网络策略（非 K8s 原生）
  kubectl get networkpolicies.crd.projectcalico.org -A 2>/dev/null
  # Calicoctl 诊断
  kubectl exec -n calico-system <calico-pod> -- calicoctl node status 2>/dev/null || e

--- (内容截断，完整内容见源文件) ---

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[26-技能/04-工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Pod 故障排查]]

## 生产案例

### 案例 1: NetworkPolicy 误配置导致 DNS 不可用

| 时间 | 事件 |
|------|------|
| 14:00 | 应用 default-deny 后所有服务 DNS 解析失败 |
| 14:02 | Pod 无法访问 kube-dns Service |
| 14:05 | NetworkPolicy 未放行到 kube-system:53 的流量 |
| 14:08 | 🟡 添加允许 DNS 的 egress 规则 |
| 14:10 | DNS 恢复 |

**根因**: default-deny 策略未包含 DNS 例外规则。

### 案例 2: 跨 namespace 访问被拒绝

**现象**: frontend 无法访问 backend namespace 的 API。

**诊断**: backend namespace 有 ingress deny 策略，未允许 frontend

**修复**: 🟢 添加 ingress 规则允许 frontend namespace 的特定 label

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 策略导致核心服务中断 | 立即删除问题策略 |
| P1 | 部分服务不可达 | 检查策略规则 |
| P2 | 策略优化 | 细化规则粒度 |

## 面试要点

1. **Q: NetworkPolicy 调试的常用工具？**
   A: ① `kubectl describe networkpolicy` 查看规则 ② `kubectl exec pod -- wget/curl` 测试连通性 ③ Calico: `calicoctl get networkpolicy` ④ Cilium: `cilium policy trace` ⑤ 网络日志/流日志。

2. **Q: 如何设计微服务间的 NetworkPolicy？**
   A: ① 先 default-deny 所有 ② 按服务依赖图逐个添加白名单 ③ 使用 label selector 而非 IP ④ 包含 DNS 例外 ⑤ 定期审计未使用规则。

3. **Q: NetworkPolicy 的性能影响？**
   A: Calico: iptables 模式 O(n)，eBPF 模式 O(1)；Cilium: eBPF 原生高性能；大规模集群(1000+ 策略)建议用 eBPF 实现，避免 iptables 规则膨胀。

## Related

- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[cilium]] — Cilium
- [[cni]] — CNI (Container Network Interface)
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
