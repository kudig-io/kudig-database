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



# [[NetworkPolicy|NetworkPolicy]] 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting

### 症状识别



### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | 新部署 Pod 无法访问已有服务 | `kubectl exec` 测试连通性 | 0.85 | 应用启动失败 → SKILL-POD-001 |
| S2 | NetworkPolicy 变更后通信中断 | 检查事件时间线 | 0.90 | 同时有其他变更 |
| S3 | 跨 namespace Pod 间通信失败 | 跨 namespace `kubectl exec` 测试 | 0.85 | DNS 问题 → SKILL-NET-001 |
| S4 | Cilium/Calico policy denied 指标升高 | Prometheus/Hubble 观测 | 0.95 | 攻击流量 → SKILL-SECURITY-001 |
| S5 | 入站正常但出站失败（或相反） | 定向连通性测试 | 0.80 | [[Service|Service]] 问题 → SKILL-NET-002 |
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

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|诊断技能总览]]
- [[skills/troubleshoot-pod-issues.md|Pod 故障排查]]

## Related

- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[cilium]] — Cilium
- [[cni]] — CNI (Container Network Interface)
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
