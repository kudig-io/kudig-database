---
title: NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting
description: '- "网络策略"'
summary: '- "网络策略"'
category: skills
tags:
- k8s
- skills
- sop
- runbook
- prometheus
- cilium
- flannel
- calico
- coredns
- ingress
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting 是什么
- 如何 NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting 故障排查
- NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting 排障步骤
trigger_keywords:
- NetworkPolicy
- 连通性故障诊断
- NetworkPolicy
- Connectivity
- Troubleshooting
- troubleshooting
- diagnostics
- skills
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- cilium-basics
- cni-basics
skill_id: SKILL-20_NETWORKPOLICY_CONNECTIVITY-001
skill_name: NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting
version: 1.0.0
---



---
skill_id: "SKILL-NET-004"
skill_name: "[[NetworkPolicy|NetworkPolicy]] 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting"
version: "1.0"
category: "network"
severity_range: "P0-P2"
k8s_versions:
  - "1.28"
  - "1.29"
  - "1.30"
  - "1.31"
  - "1.32"
estimated_resolution_time: "10-45min"
risk_level: "medium"
agent_execution_mode: "L2-semi-auto"
trigger_keywords:
  - "NetworkPolicy"
  - "网络策略"
  - "policy blocking"
  - "网络不通"
  - "连接被拒绝"
  - "connection refused"
  - "timeout"
  - "CNI policy"
  - "calico policy"
  - "[[Cilium|cilium]] policy"
trigger_events:
  - "NetworkPolicyViolation"
  - "CalicoPolicyViolation"
  - "CiliumPolicyViolation"
trigger_metrics:
  - 'cilium_policy_denied_total'
  - 'calico_policy_denied_packets'
  - 'kube_networkpolicy_*'
difficulty: "advanced"
reading_level: "advanced"
audience:
  - SRE
  - 运维工程师
  - 技术支持
estimated_read_time: "15min"
prerequisites:
  - "domain-03-networking-traffic"
  - "kubectl-basics"
  - "networking-basics"
related_skills:
  - "SKILL-NET-001"
  - "SKILL-NET-002"
  - "SKILL-NET-003"
  - "SKILL-SEC-002"
fta_refs:
  - "domain-10-troubleshooting-diagnostics/topic-fta/list/networkpolicy-fta.md"
knowledge_refs:
  - "domain-10-troubleshooting-diagnostics/16-networkpolicy-troubleshooting.md"
  - "domain-10-troubleshooting-diagnostics/25-network-connectivity-troubleshooting.md"
  - "domain-03-networking-traffic/"
cross_refs:
  - type: "fta"
    path: "../domain-10-troubleshooting-diagnostics/topic-fta/list/networkpolicy-fta.md"
    label: "NetworkPolicy 故障树分析"
  - type: "domain"
    path: "../domain-10-troubleshooting-diagnostics/16-networkpolicy-troubleshooting.md"
    label: "NetworkPolicy 深度排查"
  - type: "[[SKILL|skill]]"
    path: "../domain-10-troubleshooting-diagnostics/topic-skills/05-service-connectivity.md"
    label: "SKILL-NET-002 Service 连通性"
authors:
  - name: KUDIG Team
    role: contributor

tier: peripheral---

# NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting

NetworkPolicy 是 Kubernetes 中实现零信任网络的核心机制。当 NetworkPolicy 配置错误时，可能导致合法流量被阻断（过度限制）或安全边界失效（策略未生效）。与 Service/DNS 连通性问题不同，NetworkPolicy 问题通常在应用部署或策略变更后出现，症状表现为特定 Pod 间通信失败但 Service 和 DNS 正常。

本 Skill 覆盖 CNI 不支持、默认拒绝策略、标签选择器错误、规则定义不完整、跨 namespace 通信阻断、CNI 策略引擎异常等 10 种根因的诊断和修复。

## 何时使用此 Skill

| 症状 | 检测方法 | 置信度 |
|------|---------|--------|
| 特定 Pod 间通信失败，但同 namespace 其他 Pod 正常 | `kubectl exec` 测试连通性 | 0.85 |
| 新部署应用无法访问已有服务 | 应用部署后首次连通性测试 | 0.80 |
| NetworkPolicy 变更后服务中断 | 策略变更事件时间线 | 0.90 |
| Cilium/Calico 告警 policy denied | Prometheus 指标或 Hubble 观测 | 0.95 |
| 跨 namespace 服务调用失败 | 跨 namespace 连通性测试 | 0.85 |
| 入站/出站方向单一方向通信失败 | 定向连通性测试 | 0.80 |

**排除条件**: DNS 解析失败 → SKILL-NET-001; Service 无 Endpoint → SKILL-NET-002; Ingress 路由问题 → SKILL-NET-003; 节点级网络不通 → SKILL-NODE-001

## 快速分级（2 分钟内完成）

```
影响范围 + 业务关键度
├── 核心服务被阻断（如支付、认证）────────→ P0（立即修复）
├── 生产环境多服务通信失败───────────────→ P0（30min 内修复）
├── 单服务/非关键服务通信失败────────────→ P1（1h 内修复）
├── 新部署应用无法接入（预发布环境）──────→ P2（4h 内修复）
└── 策略未生效（安全边界失效）────────────→ P1（高优先级，但通常不阻断业务）
```

**立即升级条件**（跳过所有诊断步骤）：
- 核心服务因 NetworkPolicy 被阻断，影响用户交易
- 误配置的默认拒绝策略导致 namespace 内所有通信中断
- CNI 策略引擎完全失效（所有 NetworkPolicy 不生效）
- 安全边界失效（应被阻断的流量可通过）

## 执行流程

```
工单/告警触发
    │
    ▼
┌──────────────┐    Step: D1.1-D1.5
│ Phase 1      │    内容: kubectl 快速检查（只读，零风险）
│ 快速检查      │
└──────┬───────┘
       │ 无法确认根因
       ▼
┌──────────────┐    Step: D2.1-D2.6
│ Phase 2      │    内容: 策略规则深度分析（只读，零风险）
│ 深度检查      │
└──────┬───────┘
       │ 需主动探测
       ▼
┌──────────────┐    Step: D3.1-D3.3
│ Phase 3      │    内容: 连通性主动探测（低风险，可能需审批）
│ 主动探测      │
└──────┬───────┘
       │ 确认根因
       ▼
┌──────────────┐    RC-001~010
│ 根因匹配      │
└──────┬───────┘
       │
       ▼
┌──────────────┐    REM-001~008
│ 修复操作      │    风险: LOW → MEDIUM → HIGH → CRITICAL
└──────┬───────┘
       │
       ▼
┌──────────────┐    V1~V6
│ 验证确认      │
└──────────────┘
```

## 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | 新部署 Pod 无法访问已有服务 | `kubectl exec` 测试连通性 | 0.85 | 应用启动失败 → SKILL-POD-001 |
| S2 | NetworkPolicy 变更后通信中断 | 检查事件时间线 | 0.90 | 同时有其他变更 |
| S3 | 跨 namespace Pod 间通信失败 | 跨 namespace `kubectl exec` 测试 | 0.85 | DNS 问题 → SKILL-NET-001 |
| S4 | Cilium/Calico policy denied 指标升高 | Prometheus/Hubble 观测 | 0.95 | 攻击流量 → SKILL-SECURITY-001 |
| S5 | 入站正常但出站失败（或相反） | 定向连通性测试 | 0.80 | Service 问题 → SKILL-NET-002 |
| S6 | 所有 NetworkPolicy 似乎不生效 | 策略存在但流量未被过滤 | 0.85 | CNI 不支持 → RC-001 |
| S7 | 特定标签 Pod 通信被阻断 | 按标签分组测试连通性 | 0.85 | Pod 本身问题 → SKILL-POD-001 |
| S8 | 默认 namespace 通信正常，新 namespace 失败 | 多 namespace 对比测试 | 0.80 | namespace 配置问题 |

### 2.2 工单关键词映射

- "新部署的服务连不上数据库"
- "应用 A 无法访问应用 B，但应用 C 可以"
- "NetworkPolicy 加了之后服务就不通了"
- "跨 namespace 调用返回 connection refused"
- "Cilium 显示 policy denied"
- "某些 Pod 的网络策略不生效"
- "默认拒绝策略导致所有流量被阻断"

### 2.3 排除标准

- 所有 Pod 间通信均失败（不仅是特定 Pod）→ 可能是 CNI 级问题 → [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/03-networking-cni-troubleshooting.md|03-networking-cni-troubleshooting]].md
- DNS 解析失败 → SKILL-NET-001
- Service 无 Endpoint → SKILL-NET-002
- 节点状态 NotReady → SKILL-NODE-001
- 安全攻击流量被阻断（正常行为）→ 非问题

## 快速分级（2 分钟内完成）

### 3.1 影响评估

**Step T1**: 统计受影响 Pod 和服务的数量
```bash
# 获取通信失败的 Pod 列表（需结合工单描述）
kubectl get pods -n <namespace> -l <label-selector>
# 统计受影响 namespace 数量
kubectl get networkpolicies -A | wc -l
```
> **判断规则**: 若影响核心服务（如订单、支付）→ P0

**Step T2**: 检查 NetworkPolicy 最近变更
```bash
kubectl get events -A --field-selector reason=NetworkPolicy --sort-by=.lastTimestamp | tail -10
# 或使用 kubectl 查看策略创建/修改时间
kubectl get networkpolicy -n <namespace> -o json | jq '.items[].metadata.creationTimestamp'
```
> **判断规则**: 若问题时间与策略变更时间吻合 → 高置信度为 NetworkPolicy 问题

**Step T3**: 检查 CNI 健康状态
```bash
kubectl get pods -n calico-system -o wide 2>/dev/null || \
  kubectl get pods -n kube-system -l k8s-app=cilium -o wide 2>/dev/null || \
  echo "CNI status check needed"
```
> **判断规则**: 若 CNI Pod 不健康 → 可能不是纯策略问题，需扩展排查

**Step T4**: 快速测试连通性方向

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 从源 Pod 测试到目标 Pod
kubectl exec -n <src-ns> <src-pod> -- nc -zv <target-ip> <port>
# 从目标 Pod 测试到源 Pod（验证双向）
kubectl exec -n <target-ns> <target-pod> -- nc -zv <src-ip> <port>
```
> **判断规则**: 单向失败 → 可能是 Ingress/Egress 策略问题；双向失败 → 可能是更底层问题

### 3.2 严重性分级

| 条件 | 级别 | 说明 |
|------|------|------|
| 核心服务通信阻断 | P0 | 15min 内修复 |
| 生产环境多服务受影响 | P0 | 30min 内修复 |
| 单服务通信阻断（非核心） | P1 | 1h 内修复 |
| 新部署应用无法接入 | P2 | 4h 内修复 |
| 策略未生效（安全边界失效） | P1 | 高优先级修复 |

### 3.3 立即升级触发条件

- 默认拒绝策略导致整个 namespace 通信中断
- 核心服务（如数据库、消息队列）被阻断
- CNI 策略引擎崩溃导致所有策略失效
- 安全策略失效（应阻断的流量通过）

## 诊断工作流

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
  # 检查节点上的 CNI 配置
  kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'
  # 检查 CNI Pod
  kubectl get pods -n calico-system 2>/dev/null || kubectl get pods -n kube-system | grep -E 'calico|cilium|flannel|weave'
  ```
- **超时**: 10s
- **预期输出模式**: CNI 类型标识
- **判断规则**:
  - 使用 Flannel → RC-001（Flannel 不支持 NetworkPolicy）
  - Calico/Cilium Pod 不存在或 CrashLoopBackOff → RC-007（CNI 引擎异常）
- **版本差异**: 无

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

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # Calico 全局网络策略
  kubectl get globalnetworkpolicy 2>/dev/null
  # Calico 网络策略（非 K8s 原生）
  kubectl get networkpolicies.crd.projectcalico.org -A 2>/dev/null
  # Calicoctl 诊断
  kubectl exec -n calico-system <calico-pod> -- calicoctl node status 2>/dev/null || echo "calicoctl not available"
  ```
- **超时**: 15s
- **预期输出模式**: Calico 策略列表和节点状态
- **判断规则**:
  - 存在 GlobalNetworkPolicy 与 Namespace NetworkPolicy 冲突 → RC-006
  - Calico 节点状态异常 → RC-007
- **版本差异**: 无

**Step D2.3**: 检查 Cilium 策略状态（如使用 Cilium）
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # Cilium 端点策略状态
  kubectl exec -n kube-system <cilium-pod> -- cilium endpoint list | grep -E '<target-pod-ip>|ENDPOINT'
  # Cilium 策略列表
  kubectl exec -n kube-system <cilium-pod> -- cilium policy get 2>/dev/null | head -50
  # Hubble 观测（如已启用）
  kubectl exec -n kube-system <cilium-pod> -- hubble observe --pod <namespace>/<pod-name> 2>/dev/null | head -20
  ```
- **超时**: 15s
- **预期输出模式**: Cilium 端点策略状态和观测数据
- **判断规则**:
  - `cilium endpoint list` 显示 `Policy verdict: denied` → RC-002/003/004/005
  - Hubble 显示 `DROPPED` 且原因包含 `POLICY_DENY` → 确认策略阻断
  - Cilium 策略未加载 → RC-007
- **版本差异**: 无

**Step D2.4**: 检查 iptables 规则（通用 CNI）
- **命令**:
  ```bash
  ssh <node-ip> "iptables -t filter -L KUBE-NWPLCY -n -v --line-numbers 2>/dev/null | head -20"
  ssh <node-ip> "iptables -t filter -L | grep -i '<target-namespace>' | head -20"
  ```
- **超时**: 10s
- **预期输出模式**: iptables 规则列表
- **判断规则**:
  - 无 KUBE-NWPLCY 链 → CNI 未实施 NetworkPolicy（RC-001 或 RC-007）
  - 规则存在但计数器为 0 → 规则未匹配到流量（RC-003）
  - 规则存在且计数器增长 → 规则生效中
- **版本差异**:
  - **[v1.29+]**: nftables 模式 alpha，iptables 可能不显示规则
  - **[v1.32+]**: nftables GA，需使用 `nft list ruleset` 检查

**Step D2.5**: 检查 Pod IP 和 CIDR 匹配
- **命令**:
  ```bash
  # 源和目标 Pod IP
  kubectl get pod <src-pod> -n <src-ns> -o jsonpath='{.status.podIP}'
  kubectl get pod <target-pod> -n <target-ns> -o jsonpath='{.status.podIP}'
  # 检查 IPBlock CIDR 是否包含目标 IP
  ```
- **超时**: 10s
- **预期输出模式**: Pod IP 地址
- **判断规则**:
  - IPBlock 的 CIDR 不包含目标 Pod IP → RC-005（CIDR 配置错误）
  - 目标 Pod IP 不在预期范围内 → 可能是 IPAM 问题
- **版本差异**: 无

**Step D2.6**: 检查服务账户和身份（Cilium 等高级 CNI）
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # Cilium 身份列表
  kubectl exec -n kube-system <cilium-pod> -- cilium identity list 2>/dev/null | grep -E '<src-label>|<target-label>'
  # 检查 Pod 服务账户
  kubectl get pod <pod> -n <namespace> -o jsonpath='{.spec.serviceAccountName}'
  ```
- **超时**: 10s
- **预期输出模式**: Cilium 身份映射
- **判断规则**:
  - Cilium 身份未正确创建 → RC-007（CNI 同步问题）
  - 身份标签与策略不匹配 → RC-003
- **版本差异**: 无

### Phase 3: 主动探测（低风险，可能需审批）

**Step D3.1**: 从源 Pod 测试到目标 Pod 的连通性
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  kubectl exec -n <src-ns> <src-pod> -- nc -zv <target-pod-ip> <port>
  kubectl exec -n <src-ns> <src-pod> -- nc -zv <target-svc-name>.<target-ns>.svc.cluster.local <port>
  ```
- **超时**: 15s
- **风险级别**: 🟢 低（只读网络探测）
- **预期输出模式**: 连接成功/失败信息
- **判断规则**:
  - IP 连通但 Service 名不通 → DNS 问题（排除，转至 SKILL-NET-001）
  - IP 和 Service 都不通 → NetworkPolicy 可能阻断
  - 连接超时 vs 连接拒绝 → 超时通常是策略丢弃（DROP），拒绝通常是策略明确拒绝或应用未监听
- **版本差异**: 无

**Step D3.2**: 临时允许所有流量验证（测试策略是否生效）
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 创建临时允许所有策略（测试用）
  cat <<EOF | kubectl apply -f -
  apiVersion: networking.k8s.io/v1
  kind: NetworkPolicy
  metadata:
    name: temp-allow-all
    namespace: <namespace>
  spec:
    podSelector: {}
    policyTypes:
    - Ingress
    - Egress
    ingress:
    - {}
    egress:
    - {}
  EOF
  ```
- **超时**: 10s
- **风险级别**: 🟡 中（放宽安全策略，测试后必须删除）
- **预期输出模式**: 策略创建成功
- **判断规则**:
  - 添加 allow-all 后通信恢复 → 确认是 NetworkPolicy 限制问题
  - 添加 allow-all 后仍不通 → 可能是更底层问题（CNI/Service/DNS）
- **版本差异**: 无
- **⚠️ 重要**: 测试完成后必须删除此策略

**Step D3.3**: 测试特定规则段
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 测试特定端口
  kubectl exec -n <src-ns> <src-pod> -- nc -zv <target-ip> 80
  kubectl exec -n <src-ns> <src-pod> -- nc -zv <target-ip> 443
  # 测试特定协议（UDP）
  kubectl exec -n <src-ns> <src-pod> -- nc -uzv <target-ip> 53
  ```
- **超时**: 15s
- **风险级别**: 🟢 低（只读网络探测）
- **预期输出模式**: 各端口/协议测试结果
- **判断规则**:
  - 某些端口通，某些端口不通 → RC-005（端口规则不完整）
  - TCP 通但 UDP 不通 → RC-005（未配置 UDP 规则）
- **版本差异**: 无

## 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | CNI 插件不支持 NetworkPolicy（如 Flannel） | 中 | D1.5 CNI 类型为 Flannel；无 iptables 策略链 | cni_no_policy_support |
| RC-002 | 默认拒绝策略阻断合法流量 | 高 | D1.1 策略存在但 ingress/egress 为空；D3.2 allow-all 后恢复 | default_deny_blocking |
| RC-003 | 标签选择器（podSelector/namespaceSelector）匹配错误 | 高 | D1.2/D1.4 标签不匹配；D2.3 Cilium 身份不匹配 | selector_mismatch |
| RC-004 | 跨 namespace 通信规则配置错误 | 中 | D1.4 namespace 标签缺失；D2.5 跨 ns 测试失败 | cross_namespace_misconfig |
| RC-005 | 规则定义不完整（端口/协议/CIDR 缺失或错误） | 高 | D1.3 端口/协议不匹配；D3.3 特定端口失败 | incomplete_rules |
| RC-006 | 多个 NetworkPolicy 规则冲突 | 低 | D2.1 多个策略覆盖同一 Pod；D2.2 全局策略冲突 | policy_conflict |
| RC-007 | CNI 策略引擎异常（Calico/Cilium 问题） | 中 | D1.5 CNI Pod 不健康；D2.2/D2.3 CNI 状态异常 | cni_engine_failure |
| RC-008 | 策略未应用到目标 Pod（CNI 同步延迟） | 低 | D2.4 iptables 规则计数器为 0；新 Pod 创建后策略未生效 | policy_sync_delay |
| RC-009 | IPBlock CIDR 范围不包含目标 Pod | 低 | D2.5 IP 不在 CIDR 内 | ipblock_mismatch |
| RC-010 | 外部流量（非 Pod 流量）被策略阻断 | 中 | D1.3 无 externalIPs/nodeSelector 规则；外部访问失败 | external_traffic_blocked |

## 修复操作

### 6.1 🟢 低风险（Agent 可自动执行）

#### REM-001: 修正 Pod 标签以匹配 NetworkPolicy
- **适用根因**: RC-003
- **前置检查**:
  ```bash
  kubectl get networkpolicy <policy-name> -n <namespace> -o jsonpath='{.spec.podSelector}'
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.metadata.labels}'
  # 确认标签不匹配
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方式1: 修改 Pod 标签（需重新创建 Pod，Deployment 需修改模板）
  kubectl patch deployment <deployment-name> -n <namespace> -p \
    '{"spec":{"template":{"metadata":{"labels":{"app":"correct-label"}}}}}'
  # 或修改 NetworkPolicy 的选择器
  kubectl patch networkpolicy <policy-name> -n <namespace> -p \
    '{"spec":{"podSelector":{"matchLabels":{"app":"actual-label"}}}}'
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n <namespace> -l <correct-label>
  kubectl describe networkpolicy <policy-name> -n <namespace>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 恢复原始标签/选择器
  kubectl patch deployment <deployment-name> -n <namespace> -p \
    '{"spec":{"template":{"metadata":{"labels":{"app":"original-label"}}}}}'
  ```

#### REM-002: 补充 NetworkPolicy 规则（端口/协议/CIDR）
- **适用根因**: RC-005
- **前置检查**:
  ```bash
  kubectl get networkpolicy <policy-name> -n <namespace> -o yaml
  # 确认缺失的端口/协议
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  kubectl patch networkpolicy <policy-name> -n <namespace> --type='merge' -p '{
    "spec": {
      "ingress": [{
        "from": [{"podSelector": {}}],
        "ports": [
          {"protocol": "TCP", "port": 80},
          {"protocol": "TCP", "port": 443},
          {"protocol": "UDP", "port": 53}
        ]
      }]
    }
  }'
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  kubectl get networkpolicy <policy-name> -n <namespace> -o yaml
  kubectl exec -n <src-ns> <src-pod> -- nc -zv <target-ip> <port>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl rollout undo networkpolicy <policy-name> -n <namespace>  # 不适用，需手动恢复
  # 或重新应用原始策略 YAML
  kubectl apply -f original-policy.yaml
  ```

#### REM-003: 添加 namespace 标签以匹配 namespaceSelector
- **适用根因**: RC-004
- **前置检查**:
  ```bash
  kubectl get networkpolicy <policy-name> -n <namespace> -o jsonpath='{.spec.ingress[*].from[*].namespaceSelector}'
  kubectl get namespace <target-namespace> -o jsonpath='{.metadata.labels}'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

  ```bash
  kubectl label namespace <target-namespace> <required-label>=<value> --overwrite
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  kubectl get namespace <target-namespace> --show-labels
  kubectl exec -n <src-ns> <src-pod> -- nc -zv <target-pod-ip> <port>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

  ```bash
  kubectl label namespace <target-namespace> <required-label>-  # 删除标签
  ```

### 6.2 🟡 中风险（Agent 建议，人工审批）

#### REM-004: 创建明确的允许规则（替代默认拒绝）
- **适用根因**: RC-002
- **影响说明**: 创建新策略会改变 namespace 的网络安全边界，需确认允许范围。
- **审批提示**: "建议为 namespace <ns> 创建 NetworkPolicy 允许 <src> 访问 <target> 的 <port> 端口。是否批准？"
- **前置检查**:
  ```bash
  # 确认当前通信需求
  kubectl get pods -n <namespace> -o wide
  kubectl get svc -n <namespace>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  cat <<EOF | kubectl apply -f -
  apiVersion: networking.k8s.io/v1
  kind: NetworkPolicy
  metadata:
    name: allow-<src>-to-<target>
    namespace: <namespace>
  spec:
    podSelector:
      matchLabels:
        app: <target-app>
    policyTypes:
    - Ingress
    ingress:
    - from:
      - podSelector:
          matchLabels:
            app: <src-app>
      ports:
      - protocol: TCP
        port: <port>
  EOF
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  kubectl get networkpolicy -n <namespace>
  kubectl exec -n <src-ns> <src-pod> -- nc -zv <target-pod-ip> <port>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  kubectl delete networkpolicy allow-<src>-to-<target> -n <namespace>
  ```

#### REM-005: 添加外部流量访问规则
- **适用根因**: RC-010
- **影响说明**: 允许外部流量可能扩大攻击面，需明确限制源 IP。
- **审批提示**: "建议允许 IP 范围 <cidr> 访问 namespace <ns> 的 <port> 端口。是否批准？"
- **前置检查**:
  ```bash
  # 确认外部流量的源 IP 范围
  kubectl get networkpolicy -n <namespace> -o yaml
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  cat <<EOF | kubectl apply -f -
  apiVersion: networking.k8s.io/v1
  kind: NetworkPolicy
  metadata:
    name: allow-external
    namespace: <namespace>
  spec:
    podSelector:
      matchLabels:
        app: <target-app>
    policyTypes:
    - Ingress
    ingress:
    - from:
      - ipBlock:
          cidr: <external-cidr>
          except:
          - <blocked-subnet>
      ports:
      - protocol: TCP
        port: <port>
  EOF
  ```
- **后置验证**:
  ```bash
  kubectl get networkpolicy allow-external -n <namespace> -o yaml
  # 从外部测试访问
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  kubectl delete networkpolicy allow-external -n <namespace>
  ```

#### REM-006: 删除冲突的 NetworkPolicy
- **适用根因**: RC-006
- **影响说明**: 删除策略会改变安全边界，需确认删除的是正确的策略。
- **审批提示**: "建议删除冲突的 NetworkPolicy <policy-name>。是否批准？"
- **前置检查**:
  ```bash
  kubectl get networkpolicy -n <namespace>
  # 确认冲突策略
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 备份策略
  kubectl get networkpolicy <policy-name> -n <namespace> -o yaml > /tmp/<policy-name>-backup.yaml
  # 删除冲突策略
  kubectl delete networkpolicy <policy-name> -n <namespace>
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  kubectl get networkpolicy -n <namespace>
  kubectl exec -n <src-ns> <src-pod> -- nc -zv <target-pod-ip> <port>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  kubectl apply -f /tmp/<policy-name>-backup.yaml
  ```

### 6.3 🔴 高风险（Agent 仅提供指导）

#### REM-007: 更换支持 NetworkPolicy 的 CNI
- **适用根因**: RC-001
- **影响说明**: 更换 CNI 会导致集群网络短暂中断，需维护窗口。
- **操作步骤**:
  1. 规划维护窗口
  2. 备份现有 CNI 配置
  3. 卸载现有 CNI（如 Flannel）
  4. 安装新 CNI（如 Calico/Cilium）
  5. 验证所有节点网络恢复
  6. 部署 NetworkPolicy 并验证
- **安全检查**: 维护窗口期间确保应用可用性（多副本 + PodDisruptionBudget）
- **回滚方案**: 恢复原始 CNI 配置

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-008: 紧急禁用所有 NetworkPolicy（核选项）
- **适用根因**: RC-002/006（大规模通信阻断且无法快速定位）
- **审批要求**: 需要安全团队 + 值班经理双重审批
- **数据备份**: 导出所有 NetworkPolicy YAML
- **操作步骤**:
  1. 备份所有 NetworkPolicy
     ```bash
     kubectl get networkpolicy -A -o yaml > /tmp/all-networkpolicies-backup.yaml
     ```
  2. 删除所有 NetworkPolicy（或使用 allow-all 替代）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

     ```bash
     # 方式1: 创建 allow-all 策略覆盖每个 namespace
     for ns in $(kubectl get ns -o jsonpath='{.items[*].metadata.name}'); do
       cat <<EOF | kubectl apply -f -
       apiVersion: networking.k8s.io/v1
       kind: NetworkPolicy
       metadata:
         name: emergency-allow-all
         namespace: $ns
       spec:
         podSelector: {}
         policyTypes:
         - Ingress
         - Egress
         ingress:
         - {}
         egress:
         - {}
       EOF
     done
     ```
  3. 验证通信恢复
  4. 逐步恢复正确的 NetworkPolicy
- **回滚方案**: 重新应用备份的 NetworkPolicy YAML

## 验证确认

### 7.1 即时验证（修复后 1 分钟内）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# V1: 测试修复后的连通性
kubectl exec -n <src-ns> <src-pod> -- nc -zv <target-pod-ip> <port>
# 预期: 连接成功

# V2: 测试 Service 名连通性
kubectl exec -n <src-ns> <src-pod> -- nc -zv <svc-name>.<svc-ns>.svc.cluster.local <port>
# 预期: 连接成功

# V3: 检查 NetworkPolicy 状态
kubectl get networkpolicy -n <namespace>
# 预期: 策略存在且配置正确

# V4: 检查 Cilium/Calico 策略状态（如适用）
kubectl exec -n kube-system <cilium-pod> -- cilium policy get | grep -c "allow"
# 预期: 策略已加载
```

### 7.2 短期监控（5-15 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| Policy denied 指标 | `cilium_policy_denied_total` 或 `calico_policy_denied_packets` | 下降或稳定为 0 | 持续升高 |
| 应用错误率 | 应用监控指标 | 下降 | 持续高位 |
| 连通性探测 | 周期性 `kubectl exec` 测试 | 100% 成功 | < 100% |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：
- [ ] 源 Pod 到目标 Pod 的连通性测试通过
- [ ] 应用日志不再显示连接超时/拒绝错误
- [ ] Cilium/Calico policy denied 指标停止增长
- [ ] 相关业务功能测试通过

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| 策略变更影响 | 监控 NetworkPolicy 创建/更新事件 | 每次变更 | 变更后执行连通性测试 |
| 新部署应用 | 新 Pod 启动后连通性验证 | 每次部署 | 若失败 → 检查策略匹配 |
| Policy denied 趋势 | Prometheus 指标 | 每 4h | 若升高 → 排查新阻断 |

## 升级协议

### 8.1 自动升级条件

| 条件 | 说明 |
|------|------|
| 诊断超时 | 诊断工作流执行超过 30 分钟未确认根因 |
| 修复失败 | 同一修复操作执行 2 次仍未通过验证 |
| 大规模影响 | >50% 服务通信受影响 |
| CNI 引擎崩溃 | Calico/Cilium 无法恢复 |
| 安全边界失效 | NetworkPolicy 完全不生效 |

### 8.2 升级消息模板

```
【{severity}】{skill_name} - {cluster_name}
- 问题概述: namespace {ns} 中 {src} 无法访问 {target}
- 影响范围: {affected_services} 服务通信受阻
- 已完成诊断: {completed_steps}
- 初步发现: {findings}
- 根因候选: {root_cause_candidates}
- 需要: {action_needed}
- 工单编号: {ticket_id}
```

### 8.3 交接信息包

升级时，Agent 需准备以下信息：
1. 完整诊断路径和每步输出
2. 已排除的根因及原因
3. 涉及的 NetworkPolicy YAML（源和目标 namespace）
4. 受影响 Pod 的标签和 IP
5. CNI 类型和状态
6. 连通性测试结果汇总

## 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| NetworkPolicy API | networking.k8s.io/v1 | v1 | v1 | v1 | v1 |
| AdminNetworkPolicy | alpha | alpha | beta | beta | GA |
| Cilium ClusterMesh NetworkPolicy | 支持 | 支持 | 支持 | 支持 | 支持 |
| Calico GlobalNetworkPolicy | 支持 | 支持 | 支持 | 支持 | 支持 |

### 9.2 AdminNetworkPolicy (ANP) 说明

| 版本 | 状态 | 说明 |
|------|------|------|
| v1.28-v1.30 | alpha/beta | 集群级网络策略，优先级高于 Namespace NetworkPolicy |
| v1.31+ | beta | 多租户场景下可能覆盖 namespace 策略 |
| v1.32+ | GA | 检查 `kubectl get adminnetworkpolicy` 是否存在冲突规则 |

### 9.3 CNI 兼容性矩阵

| CNI | NetworkPolicy 支持 | 诊断工具 |
|-----|-------------------|---------|
| Calico | 完整支持 | `calicoctl`, `iptables -L` |
| Cilium | 完整支持 | `cilium policy get`, Hubble |
| Flannel | 不支持 | N/A |
| Weave Net | 支持 | `weave status` |
| Antrea | 支持 | `antctl` |

## 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| 将 DNS 问题误判为策略问题 | 服务名不通但 IP 通 | CoreDNS 被策略阻断 | 同时测试 IP 和 Service 名 |
| 将 Service 问题误判为策略问题 | 所有 Pod 不通 | Service 无 Endpoint | 检查 Service Endpoint |
| 忽略 egress 策略 | 入站通但出站不通 | 只配置了 ingress 规则 | 检查 egress 规则 |
| 混淆 podSelector 位置 | 策略不生效 | podSelector 在 spec 顶层的含义不同 | 区分 `spec.podSelector` vs `spec.ingress[].from[].podSelector` |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：
- NetworkPolicy 原理 → `domain-03-networking-traffic/`
- CNI 故障排查 → `domain-10-troubleshooting-diagnostics/03-networking-cni-troubleshooting.md`
- 网络安全架构 → `domain-01-cluster-fundamentals/14-security-architecture.md`

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-05 | v1.0 | 初始版本 | 补齐网络策略故障诊断 Skill |

## 云厂商特异性

### 11.1 ACK (Alibaba Cloud)

| 差异 | 诊断命令 | 备注 |
|------|---------|------|
| Terway 模式 | `kubectl get pods -n kube-system | grep terway` | Terway ENI 模式支持 NetworkPolicy |
| Flannel 模式 | 不支持 NetworkPolicy | 需升级到 Terway |

### 11.2 EKS (Amazon Web Services)

| 差异 | 诊断命令 | 备注 |
|------|---------|------|
| VPC CNI | 支持 NetworkPolicy (v1.14+) | 需启用 `ENABLE_NETWORK_POLICY=true` |
| Calico on EKS | 常见选择 | 使用 Tigera Operator 部署 |

### 11.3 GKE (Google Kubernetes Engine)

| 差异 | 诊断命令 | 备注 |
|------|---------|------|
| GKE Dataplane V2 | 基于 Cilium | 支持 NetworkPolicy |
| 标准模式 | 使用 Calico | 安装时可选 |

### 11.4 AKS (Azure Kubernetes Service)

| 差异 | 诊断命令 | 备注 |
|------|---------|------|
| Azure CNI | 支持 NetworkPolicy | 需在创建集群时启用 |
| Calico on AKS | 支持 | 开源 Calico 或 Azure 托管 Calico |

## 自动化集成接口

### 12.1 脚本入口

```bash
# Phase 1: 快速诊断
./scripts/diagnose-netpolicy-quick.sh --namespace <NS> --pod <POD>

# Phase 2: 深度诊断
./scripts/diagnose-netpolicy-deep.sh --namespace <NS> --src-pod <SRC> --target-pod <TARGET>

# 验证
./scripts/verify-netpolicy.sh --namespace <NS>
```

### 12.2 Webhook 回调

```yaml
receivers:
- name: skill-netpolicy-trigger
  webhook_configs:
  - url: 'http://agent-gateway/skill/SKILL-NET-004'
    send_resolved: true
```

### 12.3 输出 JSON Schema

```json
{
  "skill_id": "SKILL-NET-004",
  "namespace": "default",
  "findings": [
    { "step": "D1.1", "result": "default-deny policy exists", "severity": "high" },
    { "step": "D1.3", "result": "no ingress rules for target pod", "severity": "high" }
  ],
  "root_cause_candidates": [
    { "rc_id": "RC-002", "confidence": 0.90, "evidence": ["D1.1", "D3.2"] }
  ],
  "recommended_action": {
    "rem_id": "REM-004",
    "risk_level": "medium",
    "command": "kubectl apply -f allow-rule.yaml",
    "rollback": "kubectl delete networkpolicy allow-rule"
  }
}
```

---

*文档版本: 1.0*  
*Skill ID: SKILL-NET-004*  
*创建时间: 2026-05*  
*维护者: Kudig Team*

```