---
title: K8s 安全策略自动化研究
summary: 深入研究 Kubernetes 安全策略即代码（Policy as Code）的实践方案，覆盖 Kyverno、OPA Gatekeeper、Celery 表达式和准入控制体系。
category: research
tags:
- research
- security
- policy-as-code
- kyverno
- opa-gatekeeper
- admission-control
- compliance
tier: supporting
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
status: done
---

# K8s 安全策略自动化研究

## 研究背景

Kubernetes 安全合规要求（PCI-DSS、HIPAA、SOC2、等保 2.0）日益严格，手动审计和配置检查无法满足大规模集群需求。策略即代码（Policy as Code）通过编程方式定义安全策略，并由准入控制器自动执行，是 K8s 安全自动化的核心。

## 核心问题

1. Kyverno vs OPA Gatekeeper 的架构差异、性能和易用性对比？
2. 策略应该部署为 Validating Webhook 还是 CEL（Common Expression Language）？
3. 如何设计分阶段策略执行策略（审计→告警→强制）？
4. 合规基线（CIS Benchmark、NSA Hardening Guide）如何自动化落地？

## 调研发现

### 发现一：Kyverno vs OPA Gatekeeper

| 维度 | Kyverno | OPA Gatekeeper |
|------|---------|---------------|
| **策略语言** | YAML（原生 K8s） | Rego（专用 DSL） |
| **学习曲线** | 低（K8s 原生语法） | 高（需学 Rego） |
| **变更能力** | ✅ 修改/生成/默认值 | ❌ 仅验证 |
| **K8s 原生** | ⬤⬤⬤⬤⬤ | ⬤⬤⬤ |
| **性能** | 中（每策略一 Webhook） | 高（批量评估） |
| **策略生态** | 中（Kyverno Policies） | 高（OPA Gatekeeper Library） |
| **推荐场景** | 通用策略 + 资源修改 | 复杂逻辑 + 高性能 |

### 发现二：CEL 表达式（K8s 1.28+）

K8s 1.28 引入 ValidatingAdmissionPolicy，支持用 CEL 表达式替代 Webhook：

```yaml
# 无需 Webhook 的内置策略验证
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: require-pod-resources
spec:
  matchConstraints:
    resourceRules:
    - apiGroups: [""]
      apiVersions: ["v1"]
      operations: ["CREATE", "UPDATE"]
      resources: ["pods"]
  validations:
  - expression: "object.spec.containers.all(c, has(c.resources) && has(c.resources.requests))"
    message: "所有容器必须设置 resources.requests"
```

| 方案 | 性能 | 灵活性 | 推荐场景 |
|------|------|--------|---------|
| CEL（内置） | ⬤⬤⬤⬤⬤ | ⬤⬤⬤ | 简单策略 |
| Kyverno | ⬤⬤⬤ | ⬤⬤⬤⬤⬤ | 复杂策略+资源修改 |
| OPA Gatekeeper | ⬤⬤⬤⬤ | ⬤⬤⬤⬤⬤ | 复杂逻辑+高性能 |

### 发现三：CIS Benchmark 合规策略

```yaml
# CIS 5.4.1: 禁止特权容器
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-privileged-containers
  annotations:
    policies.kyverno.io/title: Disallow Privileged Containers
    policies.kyverno.io/category: CIS Benchmark
    policies.kyverno.io/severity: high
spec:
  validationFailureAction: Enforce
  rules:
  - name: privileged-containers
    match:
      resources:
        kinds: [Pod]
    validate:
      pattern:
        spec:
          containers:
          - securityContext:
              privileged: "false"
```

### 发现四：分阶段策略推行

```
Phase 1: 审计模式（1-2 周）
  → 策略设置为 audit（不阻断）
  → 收集违规事件
  → 分析哪些应用不合规

Phase 2: 告警模式（2-4 周）
  → 违规事件发送到 Slack/PagerDuty
  → 应用团队收到整改通知
  → 限时修复

Phase 3: 强制模式（4-6 周）
  → 策略切换为 enforce
  → 不合规配置被拒绝
  → 例外通过策略豁免管理
```

### 发现五：核心策略清单

| 策略 | CIS/NSA | 目的 |
|------|---------|------|
| 禁止特权容器 | CIS 5.2.1 | 防提权 |
| 禁止 root 用户 | CIS 5.2.2 | 降权限 |
| 必须设置 resource limits | CIS 5.2.3 | 防 DoS |
| 只读 root 文件系统 | NSA | 防篡改 |
| 禁止 hostNetwork/hostPID | CIS 5.2.4 | 网络隔离 |
| 必须设置 livenessProbe | — | 自愈 |
| 镜像必须签名 | SLSA | 供应链安全 |
| 禁止默认 namespace | — | 资源隔离 |

## 结论与建议

1. **Kyverno 是 K8s 原生策略的首选**：YAML 语法、资源变更能力、低学习曲线。
2. **CEL 适合简单策略**：K8s 1.28+ 内置，性能最优，无需部署 Webhook。
3. **OPA Gatekeeper 适合复杂逻辑**：Rego 语言强大，适合需要复杂条件判断的场景。
4. **审计→告警→强制三步走**：避免一刀切导致业务中断。
5. **CIS Benchmark 是安全基线**：至少覆盖 CIS Level 1 策略。

## 参考资料

- Kyverno: https://kyverno.io/
- OPA Gatekeeper: https://gatekeeper.op.policygovernance.org/
- CIS Kubernetes Benchmark: https://www.cisecurity.org/benchmark/kubernetes
- [[08-安全/index.md|安全目录]]
- [[25-研究/02-网络与安全/zero-trust-k8s-security.md|零信任安全架构]]

## Related

- [[24-综合/03-网络与服务网格/networkpolicy-service-mesh.md|NetworkPolicy × Service Mesh]]
- [[25-研究/02-网络与安全/supply-chain-security.md|供应链安全]]
