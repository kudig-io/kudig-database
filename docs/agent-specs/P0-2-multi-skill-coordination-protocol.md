---
title: 多技能协同协议 (Multi-Skill Coordination Protocol)
description: '## 1. 协议概述'
category: general
tags:
- k8s
- etcd
- apiserver
- kubelet
- coredns
- containerd
- ingress
- networkpolicy
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 多技能协同协议 (Multi-Skill Coordination Protocol) 是什么
- 如何 多技能协同协议 (Multi-Skill Coordination Protocol)
trigger_keywords:
- 多技能协同协议
- Multi-Skill
- Coordination
- Protocol
prerequisites:
- kubectl-basics
- etcd-basics
created: "2026-05-23"
---

# 多技能协同协议 (Multi-Skill Coordination Protocol)

> **版本**: v1.0
> **创建日期**: 2026-05-18
> **用途**: 定义当单工单涉及多组件时的 Skill 协同机制
> **依赖**: P0-1 工单分类体系与意图识别语料库

---

## 1. 协议概述

### 1.1 背景与问题

在实际工单场景中，单一问题往往涉及多个组件：

| 场景 | 涉及的 Skill |
|------|-------------|
| 节点 NotReady + 证书过期 | SKILL-NODE-001 + SKILL-SEC-001 |
| Pod Pending + DNS 解析失败 | SKILL-POD-002 + SKILL-NET-001 |
| Ingress 503 + Service 无 Endpoints | SKILL-NET-003 + SKILL-NET-002 |
| Deployment 卡住 + PVC 挂载失败 | SKILL-WORK-001 + SKILL-STORE-001 |
| 安全事件 + 节点异常 + 证书问题 | SKILL-SECURITY-001 + SKILL-NODE-001 + SKILL-SEC-001 |

### 1.2 协同模式分类

| 模式 | 说明 | 适用场景 |
|------|------|---------|
| **串行协同** | 主 Skill 完成后启动副 Skill | DNS 问题导致 Service 连通性问题 |
| **并行协同** | 主 Skill 触发副 Skill 并行执行 | 控制平面问题影响多个组件 |
| **层次协同** | 下层组件问题驱动上层组件诊断 | kubelet 问题导致 Pod 问题 |

---

## 2. 协同触发机制

### 2.1 依赖声明 (Dependency Declaration)

每个 Skill 在 YAML front matter 中声明依赖关系：

```yaml
# Skill 依赖声明
dependencies:
  # 前置依赖: 必须先完成的 Skill
  requires:
    - skill_id: "SKILL-NET-001"
      reason: "DNS 解析是 Service 连通性的前提"
      timeout: "3min"

  # 可选依赖: 诊断过程中可能需要的 Skill
  optional:
    - skill_id: "SKILL-SEC-001"
      reason: "若发现证书问题，需切换到证书处理"
      trigger_on:
        - event: "TLS handshake failure"
        - metric: "apiserver_client_certificate_expiration < 86400"

  # 被依赖: 依赖此 Skill 的其他 Skill
  depended_by:
    - skill_id: "SKILL-NET-002"
      reason: "Service 连通性依赖 DNS 解析"
    - skill_id: "SKILL-APP-SVC"
      reason: "服务访问依赖 DNS"
```

### 2.2 触发条件 (Trigger Conditions)

```yaml
trigger_conditions:
  # 自动触发: 满足条件自动启动
  auto_trigger:
    - condition: "Phase 2 发现根因属于另一 Skill 范围"
      target_skill: "SKILL-XXX-XXX"
      priority: "high"  # high | medium | low

    - condition: "修复后验证失败，疑似关联问题"
      target_skill: "SKILL-YYY-YYY"
      priority: "medium"

  # 手动触发: 需人工确认
  manual_trigger:
    - condition: "跨 Category 的复杂问题"
      target_skill: "SKILL-ZZZ-ZZZ"
      reason: "超出单一 Skill 处理范围"
```

### 2.3 结果汇总 (Result Aggregation)

当多个 Skill 协同完成时，主 Skill 负责汇总结果：

```yaml
# 结果汇总格式
coordination_result:
  primary_skill: "SKILL-NODE-001"
  participating_skills:
    - skill_id: "SKILL-NODE-001"
      role: "primary"
      findings:
        - step: "D1.3"
          result: "Node NotReady confirmed"
          root_cause: "RC-003 (kubelet certificate expired)"

    - skill_id: "SKILL-SEC-001"
      role: "secondary"
      triggered_by: "auto_trigger"
      findings:
        - step: "D2.1"
          result: "kubelet serving certificate expired"
          root_cause: "RC-001 (certificate not rotated)"
          confidence: 0.95

  consolidated_root_cause:
    primary_cause: "kubelet serving certificate expired (RC-001)"
    secondary_cause: "Node lifecycle controller triggered eviction (RC-003)"
    causal_chain:
      - "Certificate expired at 2026-05-18T10:00:00Z"
      - "kubelet cannot connect to apiserver at 10:01:00Z"
      - "Node status changed to NotReady at 10:05:00Z"
      - "Pod eviction started at 10:10:00Z"

  recommended_actions:
    - rem_id: "REM-001"
      skill_id: "SKILL-SEC-001"
      risk_level: "medium"
      sequence: 1

    - rem_id: "REM-002"
      skill_id: "SKILL-NODE-001"
      risk_level: "low"
      sequence: 2  # 在证书修复后执行
```

---

## 3. 串行协同流程

### 3.1 流程定义

```
工单输入
    │
    ▼
┌─────────────────┐
│ Skill A (主)     │ ← 执行主 Skill 诊断
│ Phase 1 → Phase 2│
└────────┬────────┘
         │ 发现依赖
         │ (e.g., DNS 问题影响 Service)
         ▼
┌─────────────────┐
│ Skill B (副)     │ ← 触发副 Skill
│ 基于 A 的上下文   │   复用 A 的发现
└────────┬────────┘
         │ B 完成
         ▼
┌─────────────────┐
│ Skill A 结果汇总  │ ← 汇总 A + B 结果
│ 生成最终报告      │
└─────────────────┘
```

### 3.2 场景示例: Pod Pending + DNS 问题

**场景**: Pod 一直处于 Pending，且 DNS 解析失败

**串行流程**:

```
Step 1: SKILL-POD-002 启动
├── D1.1: 检查 Pod 状态 → Pending confirmed
├── D1.2: 检查调度器日志 → No suitable nodes
├── D2.1: 检查节点资源 → 资源充足
├── D2.3: 检查网络配置 → 发现 NetworkPolicy
└── 结论: 需要检查 DNS 是否影响调度

Step 2: 自动触发 SKILL-NET-001
├── 接收上下文: {pod_name, namespace, node_selector}
├── D1.1: 检查 CoreDNS Pod → CoreDNS Pod NotReady
├── D2.1: 检查 CoreDNS 日志 → forward to upstream failed
└── 结论: DNS 问题导致 Pod 无法完成初始化

Step 3: SKILL-POD-002 汇总
├── 主 Skill 发现: Pod Pending
├── 副 Skill 发现: CoreDNS 不健康
└── 根因: CoreDNS 问题导致 Pod 初始化失败
```

### 3.3 上下文传递格式

```yaml
# 从 Skill A 传递到 Skill B 的上下文
context_transfer:
  from_skill: "SKILL-POD-002"
  to_skill: "SKILL-NET-001"

  # 传递的数据
  shared_context:
    namespace: "production"
    affected_workload:
      kind: "Deployment"
      name: "payment-service"
      uid: "abc-123"
    node_info:
      target_nodes: ["node-1", "node-2"]
    network_info:
      dns_server: "10.96.0.10"
      search_domains: ["svc.cluster.local", "cluster.local"]

  # 诊断状态传递
  diagnostic_state:
    completed_steps: ["D1.1", "D1.2", "D2.1"]
    findings: ["Pod Pending confirmed", "NetworkPolicy present"]
    hypothesis: "DNS resolution failure preventing Pod initialization"

  # 传递时间戳
  timestamp: "2026-05-18T10:30:00Z"
  ttl: "5m"  # 上下文有效期
```

---

## 4. 并行协同流程

### 4.1 流程定义

```
工单输入
    │
    ▼
┌─────────────────┐
│ Skill A (主)     │ ← 同时触发
│ Skill B (副)     │ ← 多个副 Skill
│ Skill C (副)     │ ← 并行执行
└────────┬────────┘
         │ 所有副 Skill 完成
         ▼
┌─────────────────┐
│ 主 Skill 结果汇总 │ ← 等待所有结果
└─────────────────┘
```

### 4.2 场景示例: 控制平面问题

**场景**: API Server 响应缓慢，影响多个组件

**并行流程**:

```
主 Skill: SKILL-CP-001 (控制平面问题)

并行触发:
├── SKILL-NET-001: 检查网络到 API Server
├── SKILL-SEC-001: 检查 API Server 证书
└── SKILL-STORE-001: 检查 etcd 存储延迟

结果汇总:
├── NET-001: 网络延迟 200ms（正常范围外）
├── SEC-001: 证书即将过期（30天内）
└── STORE-001: etcd 写入延迟 50ms

结论: 证书即将过期导致 TLS 握手延迟，进而影响 API Server 响应
```

### 4.3 并行协调规则

| 规则 | 说明 |
|------|------|
| 超时控制 | 所有副 Skill 需在主 Skill 的 Phase 2 完成前返回结果 |
| 优先级 | 副 Skill 按声明顺序执行，若超时则标记为"未完成" |
| 冲突检测 | 若两个副 Skill 结论矛盾，主 Skill 优先级高 |
| 结果合并 | 所有副 Skill 结果合并，按置信度排序 |

---

## 5. 层次协同流程

### 5.1 层次定义

```
┌─────────────────────────────────┐
│  Layer 3: 应用层 (Pod/Workload)  │ ← 上层组件
├─────────────────────────────────┤
│  Layer 2: 网络层 (DNS/Service)   │
├─────────────────────────────────┤
│  Layer 1: 基础设施 (Node/CNI)   │ ← 下层组件
└─────────────────────────────────┘
```

**层次协同原则**: 下层组件问题驱动上层组件诊断

- Node NotReady → Pod 异常 (驱动 SKILL-POD-001)
- kubelet 问题 → containerd 问题 (驱动 SKILL-NODE-001)
- CNI 问题 → Pod 网络不通 (驱动 SKILL-NET-001)

### 5.2 驱动规则

```yaml
bottom_up_escalation:
  rules:
    - trigger: "Node condition shows NotReady"
      driven_skill: "SKILL-POD-001"
      reason: "Pod will be evicted due to node failure"

    - trigger: "CoreDNS Pod not healthy"
      driven_skill: "SKILL-NET-001"
      reason: "DNS failure affects all Pods with cluster domain"

    - trigger: "CSI driver Pod not running"
      driven_skill: "SKILL-STORE-001"
      reason: "Storage operations depend on CSI driver"

    - trigger: "kube-proxy not healthy"
      driven_skill: "SKILL-NET-002"
      reason: "Service connectivity depends on kube-proxy"
```

---

## 6. 协同结果格式

### 6.1 统一输出格式 (JSON)

```json
{
  "ticket_id": "TKT-20260518-001",
  "primary_skill_id": "SKILL-NODE-001",
  "coordination_mode": "serial|parallel|hierarchical",
  "participating_skills": [
    {
      "skill_id": "SKILL-NODE-001",
      "role": "primary",
      "status": "completed",
      "execution_time_ms": 45000,
      "findings": [
        {
          "step": "D1.3",
          "result": "kubelet serving certificate expired",
          "confidence": 0.95
        }
      ],
      "root_cause_id": "RC-001"
    },
    {
      "skill_id": "SKILL-SEC-001",
      "role": "secondary",
      "status": "completed",
      "execution_time_ms": 30000,
      "findings": [
        {
          "step": "D2.1",
          "result": "certificate not rotated since 2026-04-01",
          "confidence": 0.90
        }
      ],
      "root_cause_id": "RC-001"
    }
  ],
  "consolidated_analysis": {
    "primary_root_cause": {
      "cause_id": "RC-001",
      "description": "kubelet serving certificate expired",
      "confidence": 0.95,
      "evidence": ["SKILL-NODE-001:D1.3", "SKILL-SEC-001:D2.1"]
    },
    "secondary_causes": [
      {
        "cause_id": "RC-003",
        "description": "Node lifecycle controller triggered eviction",
        "confidence": 0.80,
        "triggered_by": "RC-001"
      }
    ],
    "causal_chain": [
      "Certificate expired at 2026-05-18T10:00:00Z",
      "kubelet cannot connect to apiserver at 10:01:00Z",
      "Node status changed to NotReady at 10:05:00Z",
      "Pod eviction started at 10:10:00Z"
    ]
  },
  "recommended_actions": [
    {
      "rem_id": "REM-001",
      "skill_id": "SKILL-SEC-001",
      "description": "Rotate kubelet serving certificate",
      "risk_level": "medium",
      "sequence": 1,
      "estimated_time": "5min"
    },
    {
      "rem_id": "REM-002",
      "skill_id": "SKILL-NODE-001",
      "description": "Uncordon node after certificate rotation",
      "risk_level": "low",
      "sequence": 2,
      "estimated_time": "2min"
    }
  ],
  "next_actions": {
    "verify_skill_id": "SKILL-NODE-001",
    "verify_section": "7.1",
    "monitoring_period": "15min"
  }
}
```

---

## 7. 冲突解决规则

### 7.1 冲突类型

| 冲突类型 | 说明 | 解决规则 |
|---------|------|---------|
| 结论冲突 | 两个 Skill 给出相反的根因 | 主 Skill 结论优先；同等优先级时置信度高者优先 |
| 修复冲突 | 两个修复操作互相抵触 | 按 Skill 优先级执行；冲突操作需升级审批 |
| 范围冲突 | 根因跨越两个 Skill 边界 | 归类到更上层的 Skill（如应用层优于基础设施层） |

### 7.2 优先级矩阵

| Skill 类别 | 优先级 | 说明 |
|-----------|--------|------|
| TC-SEC-INCIDENT | P0 | 安全事件优先处理 |
| TC-INFRA-CP | P0.5 | 控制平面优先于工作负载 |
| TC-INFRA-NODE | P1 | 节点问题优先于应用问题 |
| TC-APP-* | P2 | 应用层问题标准优先级 |
| TC-DATA-* | P1.5 | 数据层问题高优先级 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 |
|------|------|
| 协同失败 | 3 个以上 Skill 协同仍未确认根因 |
| 冲突无法解决 | 结论冲突且置信度相同 |
| 修复失败 | 协同执行修复后 2 次验证失败 |
| 超时 | 总协同时间超过 30 分钟 |

### 8.2 升级信息包

```yaml
escalation_package:
  ticket_id: "TKT-20260518-001"
  escalation_reason: "multi-skill coordination failed"

  skills_attempted:
    - skill_id: "SKILL-NODE-001"
      status: "completed"
      findings: ["certificate expired"]

    - skill_id: "SKILL-SEC-001"
      status: "completed"
      findings: ["certificate rotation failed"]

  conflicts:
    - type: "修复冲突"
      description: "REM-001 要求重启 kubelet，但会导致服务中断"
      resolution_needed: true

  causal_chain_established: true
  recommended_actions: ["manual certificate rotation", "node drain before restart"]

  human_action_required:
    - "批准 kubelet 重启操作"
    - "确认服务中断窗口"
```

---

## 9. 实现检查清单

### 9.1 Skill 开发者检查项

在编写新 Skill 时，必须完成以下协同协议检查：

- [ ] 声明 `dependencies.requires`（若此 Skill 依赖其他 Skill）
- [ ] 声明 `dependencies.optional`（若可能触发其他 Skill）
- [ ] 声明 `trigger_conditions.auto_trigger`（跨 Skill 触发条件）
- [ ] 实现 `context_transfer` 格式（接收和发送上下文）
- [ ] 实现 `coordination_result` 格式（输出协同结果）
- [ ] 测试串行/并行/层次协同场景
- [ ] 更新 `P0-1-ticket-classification-intent-recognition.md` 的路由表

### 9.2 协同测试场景

| 场景 | 测试目标 | 验收标准 |
|------|---------|---------|
| Pod Pending + DNS 问题 | 串行协同 | SKILL-POD-002 正确触发 SKILL-NET-001 |
| 控制平面问题 | 并行协同 | NET/CERT/STORE 同时执行，结果汇总正确 |
| Node NotReady + 证书过期 | 层次协同 | 上层 Pod 异常被下层 Node 问题解释 |
| 修复冲突 | 冲突解决 | 高优先级 Skill 修复被优先执行 |

---

**关联文档**:
- [P0-1: 工单分类体系与意图识别语料库](./P0-1-ticket-classification-intent-recognition.md)
- [P0-3: 会话上下文管理机制](./P0-3-session-context-management.md)
- [domain-10-troubleshooting-diagnostics/[[domain-04-storage-data/README|README]].md](../domain-10-troubleshooting-diagnostics/topic-skills/README.md)
- [templates/skill-template.md](../templates/skill-template.md)