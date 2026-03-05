# topic-skills — 工单智能体 Kubernetes 诊断 Skill 库

> **适用版本**: Kubernetes v1.28 - v1.32  
> **Skill 数量**: 6 个核心场景（第一批）  
> **定位**: 面向 AI Agent 运行时的自包含工单处理 Runbook  
> **最后更新**: 2026-03

---

## 1. 什么是 Skill？

Skill 是工单智能体（Ticket Handling Agent）在运行时可直接调用的**自包含诊断-修复执行单元**。每个 Skill 覆盖一类特定的 Kubernetes 故障场景，包含从症状识别到修复验证的完整闭环。

### 与现有知识资产的定位区分

```
┌─────────────────────────────────────────────────────────────────┐
│  Layer 4: topic-skills/         (做什么 — Agent 执行层)          │
│  自包含 Runbook：症状触发 → 诊断 → 修复 → 验证 → 升级            │
├─────────────────────────────────────────────────────────────────┤
│  Layer 3: topic-fta/list/       (为什么 — 故障分析模型层)         │
│  FTA 故障树：概率模型、因果链、底事件分解                          │
├─────────────────────────────────────────────────────────────────┤
│  Layer 2: topic-structural-     (怎么查 — 深度排查参考层)         │
│           trouble-shooting/                                      │
├─────────────────────────────────────────────────────────────────┤
│  Layer 1: domain-*/             (背景知识 — 理论与架构层)          │
│  组件架构、设计原理、理论基础                                     │
└─────────────────────────────────────────────────────────────────┘
```

| 维度 | topic-fta/list/ | topic-structural-trouble-shooting/ | **topic-skills/** |
|------|----------------|-----------------------------------|-------------------|
| **定位** | 故障树分析模型 | 人类可读深度排查指南 | Agent 可执行工单处理 Runbook |
| **结构** | Mermaid 图 + JSON 工作流 | 决策树 + 解释性文字 | YAML 元数据 + 症状触发 + 分步诊断 + 风险分级修复 |
| **受众** | FTA 分析师 / Agent 推理引擎 | 初级到高级运维人员 | AI Agent 运行时（工单处理循环） |
| **粒度** | 按组件（37 个） | 按组件（40+ 文档） | 按故障场景（高频工单类型） |
| **输出** | 根因路径 + 概率 | 解释 + 命令 | 结构化动作序列 + 风险门控 + 验证关卡 |

---

## 2. Skill 索引表

| Skill ID | 文件 | 名称 | 分类 | 严重性 | 风险等级 | Agent 模式 |
|----------|------|------|------|--------|---------|-----------|
| SKILL-NODE-001 | [01-node-notready.md](./01-node-notready.md) | 节点 NotReady 诊断与修复 | node | P0-P2 | high | L1-advisory |
| SKILL-POD-001 | [02-pod-crashloop-oomkilled.md](./02-pod-crashloop-oomkilled.md) | Pod CrashLoopBackOff & OOMKilled | pod | P1-P3 | medium | L2-semi-auto |
| SKILL-POD-002 | [03-pod-pending.md](./03-pod-pending.md) | Pod Pending / 调度失败 | pod | P1-P3 | low | L2-semi-auto |
| SKILL-NET-001 | [04-dns-resolution-failure.md](./04-dns-resolution-failure.md) | DNS 解析故障 | network | P0-P2 | medium | L2-semi-auto |
| SKILL-NET-002 | [05-service-connectivity.md](./05-service-connectivity.md) | Service 连通性 / Endpoint 异常 | network | P0-P2 | medium | L2-semi-auto |
| SKILL-SEC-001 | [06-certificate-expiry.md](./06-certificate-expiry.md) | 证书过期 & TLS 故障 | security | P0-P1 | critical | L1-advisory |

---

## 3. 症状 → Skill 快速查找

### 按错误现象查找

| 常见现象 / 告警 | 对应 Skill | 置信度 |
|----------------|-----------|--------|
| `kubectl get nodes` 显示 NotReady | [01-node-notready](./01-node-notready.md) | 0.95 |
| 节点状态频繁在 Ready/NotReady 间切换 | [01-node-notready](./01-node-notready.md) | 0.85 |
| Pod 状态显示 CrashLoopBackOff | [02-pod-crashloop-oomkilled](./02-pod-crashloop-oomkilled.md) | 0.95 |
| Pod 被 OOMKilled (exit code 137) | [02-pod-crashloop-oomkilled](./02-pod-crashloop-oomkilled.md) | 0.95 |
| Pod 长期处于 Pending 状态 | [03-pod-pending](./03-pod-pending.md) | 0.95 |
| Events 中出现 FailedScheduling | [03-pod-pending](./03-pod-pending.md) | 0.90 |
| 容器内 DNS 解析失败 (NXDOMAIN / timeout) | [04-dns-resolution-failure](./04-dns-resolution-failure.md) | 0.95 |
| CoreDNS Pod 不健康或频繁重启 | [04-dns-resolution-failure](./04-dns-resolution-failure.md) | 0.85 |
| Service ClusterIP 无法访问 | [05-service-connectivity](./05-service-connectivity.md) | 0.90 |
| Endpoints 为空 / EndpointSlice 无条目 | [05-service-connectivity](./05-service-connectivity.md) | 0.90 |
| `x509: certificate has expired` 错误 | [06-certificate-expiry](./06-certificate-expiry.md) | 0.95 |
| TLS handshake failure | [06-certificate-expiry](./06-certificate-expiry.md) | 0.80 |
| kubelet 无法连接 apiserver | [01-node-notready](./01-node-notready.md) + [06-certificate-expiry](./06-certificate-expiry.md) | 0.70 |

### 按 Kubernetes Event Reason 查找

| Event Reason | 对应 Skill |
|-------------|-----------|
| `NodeNotReady`, `NodeStatusUnknown` | 01-node-notready |
| `KubeletNotReady`, `NodeHasDiskPressure`, `NodeHasMemoryPressure`, `NodeHasPIDPressure` | 01-node-notready |
| `BackOff`, `Killing` (OOMKilled) | 02-pod-crashloop-oomkilled |
| `FailedScheduling`, `Unschedulable` | 03-pod-pending |
| DNS 相关 error message | 04-dns-resolution-failure |
| `FailedToUpdateEndpoint`, `FailedToUpdateEndpointSlices` | 05-service-connectivity |
| TLS / x509 相关 error | 06-certificate-expiry |

### 按 Prometheus 告警查找

| 告警规则 / 指标模式 | 对应 Skill |
|-------------------|-----------|
| `kube_node_status_condition{condition="Ready",status="false"}` | 01-node-notready |
| `kube_node_status_condition{condition="MemoryPressure",status="true"}` | 01-node-notready |
| `kube_pod_container_status_waiting_reason{reason="CrashLoopBackOff"}` | 02-pod-crashloop-oomkilled |
| `kube_pod_container_status_last_terminated_reason{reason="OOMKilled"}` | 02-pod-crashloop-oomkilled |
| `kube_pod_status_phase{phase="Pending"} > 0` (持续 > 5min) | 03-pod-pending |
| `coredns_dns_responses_total{rcode="SERVFAIL"}` rate 升高 | 04-dns-resolution-failure |
| `kube_endpoint_address_available == 0` | 05-service-connectivity |
| `apiserver_client_certificate_expiration_seconds < 86400` | 06-certificate-expiry |

---

## 4. Agent 集成指南

### 4.1 Skill 路由（Intent → Skill Matching）

```
工单/告警输入
    │
    ▼
┌─────────────────┐
│ 1. 关键词匹配     │ ← trigger_keywords (YAML front matter)
│ 2. Event 匹配    │ ← trigger_events
│ 3. Metric 匹配   │ ← trigger_metrics
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 症状识别表验证    │ ← Section 2: 置信度 > 阈值 + 排除标准检查
└────────┬────────┘
         │ 选中 Skill
         ▼
┌─────────────────┐
│ 快速分级 (2min)  │ ← Section 3: 影响评估 + P0-P3 分级
└────────┬────────┘
         │
    ┌────┴────┐
    │ P0/P1?  │──Yes──→ 检查立即升级条件
    └────┬────┘
         │ No
         ▼
┌─────────────────┐
│ 诊断工作流       │ ← Section 4: Phase 1 → 2 → 3
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 根因确认         │ ← Section 5: 匹配根因分类表
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 修复操作         │ ← Section 6: 按风险等级执行/建议/升级
│ 🟢低风险→自动    │
│ 🟡中风险→审批    │
│ 🔴高风险→指导    │
│ ⚫严重→升级      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 验证确认         │ ← Section 7: 即时 + 短期 + 回归
└─────────────────┘
```

### 4.2 YAML Front Matter 解析

每个 Skill 以 YAML front matter 开头，Agent 应解析以下关键字段进行路由决策：

```yaml
# 必选字段
skill_id: string          # 唯一标识，格式 SKILL-{CATEGORY}-{SEQ}
skill_name: string        # 中英文双语名称
category: string          # node | pod | network | storage | control-plane | security
severity_range: string    # 适用严重性范围，如 P0-P2
k8s_versions: list        # 兼容的 K8s 版本列表
risk_level: string        # low | medium | high | critical
agent_execution_mode: string  # L1-advisory | L2-semi-auto | L3-full-auto

# 路由匹配字段
trigger_keywords: list    # NLP 匹配关键词（中英文）
trigger_events: list      # Kubernetes Event Reason
trigger_metrics: list     # Prometheus 指标模式

# 关联引用
related_skills: list      # 关联 Skill ID
fta_refs: list            # 对应 FTA 文件路径
knowledge_refs: list      # 深度知识参考路径
```

### 4.3 风险门控（Human-in-the-Loop）

| Agent 模式 | 🟢 低风险 | 🟡 中风险 | 🔴 高风险 | ⚫ 严重 |
|-----------|----------|----------|----------|--------|
| **L1-advisory** | 建议 | 建议 | 建议 | 升级 |
| **L2-semi-auto** | 自动执行 | 人工审批后执行 | 建议 | 升级 |
| **L3-full-auto** | 自动执行 | 自动执行 | 人工审批后执行 | 升级 |

### 4.4 反馈闭环

Agent 在执行 Skill 后应记录：
1. **诊断路径**: 实际执行了哪些 Step、每步的输出摘要
2. **根因确认**: 最终确认的根因 ID 及置信度
3. **修复结果**: 执行了哪个修复操作、是否成功
4. **验证状态**: 即时验证是否通过
5. **新发现**: 诊断过程中发现的未在 Skill 中覆盖的情况 → 反馈到知识进化

---

## 5. Kubernetes 版本兼容总表 (v1.28 - v1.32)

### 影响 Skills 的关键版本变更

| 版本 | 关键变更 | 影响的 Skill |
|------|---------|-------------|
| **v1.28** | Native Sidecar Containers (alpha); `kubectl debug` 增强 ephemeral containers; ValidatingAdmissionPolicy (beta) | 02-pod-crashloop, 03-pod-pending |
| **v1.29** | ReadWriteOncePod GA; KMS v2 GA; nftables kube-proxy (alpha); load balancer IP mode API | 05-service-connectivity, 06-certificate-expiry |
| **v1.30** | Node swap support (beta); Structured auth config GA; CEL for admission; HPA container resource metrics | 01-node-notready, 05-service-connectivity |
| **v1.31** | AppArmor GA; Multiple service CIDRs (beta); Traffic distribution for Services; Consistent reads from cache | 05-service-connectivity |
| **v1.32** | Auto-remove PV claim policy; Structured authorization config; Custom resource field selectors GA; 改进的 Pod scheduling readiness | 03-pod-pending |

### kubectl debug 可用性矩阵

| 功能 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| Ephemeral Containers | GA | GA | GA | GA | GA |
| Node Debug (`kubectl debug node/`) | GA | GA | GA | GA | GA |
| Custom Debug Profiles | beta | beta | GA | GA | GA |
| Sidecar Container Debug | alpha | beta | beta | GA | GA |

### kube-proxy 模式矩阵

| 模式 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| iptables | 默认 | 默认 | 默认 | 默认 | 默认 |
| IPVS | 稳定 | 稳定 | 稳定 | 稳定 | 稳定 |
| nftables | - | alpha | beta | beta | GA |

---

## 6. 关联资源

| 资源 | 路径 | 用途 |
|------|------|------|
| FTA 故障树库 | [topic-fta/list/](../topic-fta/list/) | 每个 Skill 对应的故障分析模型 |
| FEBM 循证方法论 | [topic-febm/](../topic-febm/) | Agent 工单处理的理论基础 |
| 结构化故障排查 | [topic-structural-trouble-shooting/](../topic-structural-trouble-shooting/) | 深度排查参考指南 |
| Agent 设计 | [topic-agent/](../topic-agent/) | Agent 架构与设计模式 |
| 事件管理 Runbook | [topic-dictionary/12-incident-management-runbooks.md](../topic-dictionary/12-incident-management-runbooks.md) | 事件管理流程模板 |
| 生产排障 Playbook | [topic-dictionary/16-production-troubleshooting-playbook.md](../topic-dictionary/16-production-troubleshooting-playbook.md) | 生产环境排障手册 |
| Skill 文档模板 | [_skill-schema.md](./_skill-schema.md) | 新建 Skill 的规范化模板 |

---

## 7. 后续规划

第二批 6 个 Skill（按优先级排序）：

| 优先级 | Skill ID | 场景 | 分类 |
|--------|---------|------|------|
| P1 | SKILL-STORE-001 | PVC/存储故障 | storage |
| P1 | SKILL-WORK-001 | Deployment 滚动更新卡住 | workload |
| P1 | SKILL-SEC-002 | RBAC 权限拒绝 | security |
| P2 | SKILL-SCALE-001 | HPA 弹性伸缩故障 | scaling |
| P2 | SKILL-CP-001 | etcd 集群降级 | control-plane |
| P2 | SKILL-NET-003 | Ingress/Gateway 路由故障 | network |
